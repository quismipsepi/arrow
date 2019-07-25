/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.arrow.vector;

import static org.apache.arrow.vector.TestUtils.newVarBinaryVector;
import static org.apache.arrow.vector.TestUtils.newVarCharVector;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.vector.complex.ListVector;
import org.apache.arrow.vector.complex.StructVector;
import org.apache.arrow.vector.complex.UnionVector;
import org.apache.arrow.vector.complex.impl.NullableStructWriter;
import org.apache.arrow.vector.complex.impl.UnionListWriter;
import org.apache.arrow.vector.dictionary.Dictionary;
import org.apache.arrow.vector.dictionary.DictionaryEncoder;
import org.apache.arrow.vector.holders.NullableIntHolder;
import org.apache.arrow.vector.holders.NullableUInt4Holder;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.pojo.ArrowType;
import org.apache.arrow.vector.types.pojo.DictionaryEncoding;
import org.apache.arrow.vector.types.pojo.FieldType;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class TestDictionaryVector {

  private BufferAllocator allocator;

  byte[] zero = "foo".getBytes(StandardCharsets.UTF_8);
  byte[] one = "bar".getBytes(StandardCharsets.UTF_8);
  byte[] two = "baz".getBytes(StandardCharsets.UTF_8);

  byte[][] data = new byte[][] {zero, one, two};

  @Before
  public void init() {
    allocator = new DirtyRootAllocator(Long.MAX_VALUE, (byte) 100);
  }

  @After
  public void terminate() throws Exception {
    allocator.close();
  }

  @Test
  public void testEncodeStrings() {
    // Create a new value vector
    try (final VarCharVector vector = newVarCharVector("foo", allocator);
        final VarCharVector dictionaryVector = newVarCharVector("dict", allocator);) {
      vector.allocateNew(512, 5);

      // set some values
      vector.setSafe(0, zero, 0, zero.length);
      vector.setSafe(1, one, 0, one.length);
      vector.setSafe(2, one, 0, one.length);
      vector.setSafe(3, two, 0, two.length);
      vector.setSafe(4, zero, 0, zero.length);
      vector.setValueCount(5);

      // set some dictionary values
      dictionaryVector.allocateNew(512, 3);
      dictionaryVector.setSafe(0, zero, 0, zero.length);
      dictionaryVector.setSafe(1, one, 0, one.length);
      dictionaryVector.setSafe(2, two, 0, two.length);
      dictionaryVector.setValueCount(3);

      Dictionary dictionary =
          new Dictionary(dictionaryVector, new DictionaryEncoding(1L, false, null));

      try (final ValueVector encoded = (FieldVector) DictionaryEncoder.encode(vector, dictionary)) {
        // verify indices
        assertEquals(IntVector.class, encoded.getClass());

        IntVector index = ((IntVector) encoded);
        assertEquals(5, index.getValueCount());
        assertEquals(0, index.get(0));
        assertEquals(1, index.get(1));
        assertEquals(1, index.get(2));
        assertEquals(2, index.get(3));
        assertEquals(0, index.get(4));

        // now run through the decoder and verify we get the original back
        try (ValueVector decoded = DictionaryEncoder.decode(encoded, dictionary)) {
          assertEquals(vector.getClass(), decoded.getClass());
          assertEquals(vector.getValueCount(), ((VarCharVector) decoded).getValueCount());
          for (int i = 0; i < 5; i++) {
            assertEquals(vector.getObject(i), ((VarCharVector) decoded).getObject(i));
          }
        }
      }
    }
  }

  @Test
  public void testEncodeLargeVector() {
    // Create a new value vector
    try (final VarCharVector vector = newVarCharVector("foo", allocator);
        final VarCharVector dictionaryVector = newVarCharVector("dict", allocator);) {
      vector.allocateNew();

      int count = 10000;

      for (int i = 0; i < 10000; ++i) {
        vector.setSafe(i, data[i % 3], 0, data[i % 3].length);
      }
      vector.setValueCount(count);

      dictionaryVector.allocateNew(512, 3);
      dictionaryVector.setSafe(0, zero, 0, zero.length);
      dictionaryVector.setSafe(1, one, 0, one.length);
      dictionaryVector.setSafe(2, two, 0, two.length);
      dictionaryVector.setValueCount(3);

      Dictionary dictionary =
          new Dictionary(dictionaryVector, new DictionaryEncoding(1L, false, null));


      try (final ValueVector encoded = (FieldVector) DictionaryEncoder.encode(vector, dictionary)) {
        // verify indices
        assertEquals(IntVector.class, encoded.getClass());

        IntVector index = ((IntVector) encoded);
        assertEquals(count, index.getValueCount());
        for (int i = 0; i < count; ++i) {
          assertEquals(i % 3, index.get(i));
        }

        // now run through the decoder and verify we get the original back
        try (ValueVector decoded = DictionaryEncoder.decode(encoded, dictionary)) {
          assertEquals(vector.getClass(), decoded.getClass());
          assertEquals(vector.getValueCount(), decoded.getValueCount());
          for (int i = 0; i < count; ++i) {
            assertEquals(vector.getObject(i), decoded.getObject(i));
          }
        }
      }
    }
  }

  @Test
  public void testEncodeList() {
    // Create a new value vector
    try (final ListVector vector = ListVector.empty("vector", allocator);
        final ListVector dictionaryVector = ListVector.empty("dict", allocator);) {

      UnionListWriter writer = vector.getWriter();
      writer.allocate();

      //set some values
      writeListVector(writer, new int[]{10, 20});
      writeListVector(writer, new int[]{10, 20});
      writeListVector(writer, new int[]{10, 20});
      writeListVector(writer, new int[]{30, 40, 50});
      writeListVector(writer, new int[]{30, 40, 50});
      writeListVector(writer, new int[]{10, 20});

      writer.setValueCount(6);

      UnionListWriter dictWriter = dictionaryVector.getWriter();
      dictWriter.allocate();

      writeListVector(dictWriter, new int[]{10, 20});
      writeListVector(dictWriter, new int[]{30, 40, 50});

      dictWriter.setValueCount(2);

      Dictionary dictionary = new Dictionary(dictionaryVector, new DictionaryEncoding(1L, false, null));

      try (final ValueVector encoded = (FieldVector) DictionaryEncoder.encode(vector, dictionary)) {
        // verify indices
        assertEquals(IntVector.class, encoded.getClass());

        IntVector index = ((IntVector)encoded);
        assertEquals(6, index.getValueCount());
        assertEquals(0, index.get(0));
        assertEquals(0, index.get(1));
        assertEquals(0, index.get(2));
        assertEquals(1, index.get(3));
        assertEquals(1, index.get(4));
        assertEquals(0, index.get(5));

        // now run through the decoder and verify we get the original back
        try (ValueVector decoded = DictionaryEncoder.decode(encoded, dictionary)) {
          assertEquals(vector.getClass(), decoded.getClass());
          assertEquals(vector.getValueCount(), decoded.getValueCount());
          for (int i = 0; i < 5; i++) {
            assertEquals(vector.getObject(i), decoded.getObject(i));
          }
        }
      }
    }
  }

  @Test
  public void testEncodeStruct() {
    // Create a new value vector
    try (final StructVector vector = StructVector.empty("vector", allocator);
        final StructVector dictionaryVector = StructVector.empty("dict", allocator);) {
      vector.addOrGet("f0", FieldType.nullable(new ArrowType.Int(32, true)), IntVector.class);
      vector.addOrGet("f1", FieldType.nullable(new ArrowType.Int(64, true)), BigIntVector.class);
      dictionaryVector.addOrGet("f0", FieldType.nullable(new ArrowType.Int(32, true)), IntVector.class);
      dictionaryVector.addOrGet("f1", FieldType.nullable(new ArrowType.Int(64, true)), BigIntVector.class);

      NullableStructWriter writer = vector.getWriter();
      writer.allocate();

      writeStructVector(writer, 1, 10L);
      writeStructVector(writer, 1, 10L);
      writeStructVector(writer, 1, 10L);
      writeStructVector(writer, 2, 20L);
      writeStructVector(writer, 2, 20L);
      writeStructVector(writer, 2, 20L);
      writeStructVector(writer, 1, 10L);

      writer.setValueCount(7);

      NullableStructWriter dictWriter = dictionaryVector.getWriter();
      dictWriter.allocate();

      writeStructVector(dictWriter, 1, 10L);
      writeStructVector(dictWriter, 2, 20L);


      dictionaryVector.setValueCount(2);

      Dictionary dictionary = new Dictionary(dictionaryVector, new DictionaryEncoding(1L, false, null));

      try (final ValueVector encoded = DictionaryEncoder.encode(vector, dictionary)) {
        // verify indices
        assertEquals(IntVector.class, encoded.getClass());

        IntVector index = ((IntVector)encoded);
        assertEquals(7, index.getValueCount());
        assertEquals(0, index.get(0));
        assertEquals(0, index.get(1));
        assertEquals(0, index.get(2));
        assertEquals(1, index.get(3));
        assertEquals(1, index.get(4));
        assertEquals(1, index.get(5));
        assertEquals(0, index.get(6));

        // now run through the decoder and verify we get the original back
        try (ValueVector decoded = DictionaryEncoder.decode(encoded, dictionary)) {
          assertEquals(vector.getClass(), decoded.getClass());
          assertEquals(vector.getValueCount(), decoded.getValueCount());
          for (int i = 0; i < 5; i++) {
            assertEquals(vector.getObject(i), decoded.getObject(i));
          }
        }
      }
    }
  }

  @Test
  public void testEncodeBinaryVector() {
    // Create a new value vector
    try (final VarBinaryVector vector = newVarBinaryVector("foo", allocator);
        final VarBinaryVector dictionaryVector = newVarBinaryVector("dict", allocator);) {
      vector.allocateNew(512, 5);

      // set some values
      vector.setSafe(0, zero, 0, zero.length);
      vector.setSafe(1, one, 0, one.length);
      vector.setSafe(2, one, 0, one.length);
      vector.setSafe(3, two, 0, two.length);
      vector.setSafe(4, zero, 0, zero.length);
      vector.setValueCount(5);

      // set some dictionary values
      dictionaryVector.allocateNew(512, 3);
      dictionaryVector.setSafe(0, zero, 0, zero.length);
      dictionaryVector.setSafe(1, one, 0, one.length);
      dictionaryVector.setSafe(2, two, 0, two.length);
      dictionaryVector.setValueCount(3);

      Dictionary dictionary = new Dictionary(dictionaryVector, new DictionaryEncoding(1L, false, null));

      try (final ValueVector encoded = DictionaryEncoder.encode(vector, dictionary)) {
        // verify indices
        assertEquals(IntVector.class, encoded.getClass());

        IntVector index = ((IntVector)encoded);
        assertEquals(5, index.getValueCount());
        assertEquals(0, index.get(0));
        assertEquals(1, index.get(1));
        assertEquals(1, index.get(2));
        assertEquals(2, index.get(3));
        assertEquals(0, index.get(4));

        // now run through the decoder and verify we get the original back
        try (VarBinaryVector decoded = (VarBinaryVector) DictionaryEncoder.decode(encoded, dictionary)) {
          assertEquals(vector.getClass(), decoded.getClass());
          assertEquals(vector.getValueCount(), decoded.getValueCount());
          for (int i = 0; i < 5; i++) {
            assertTrue(Arrays.equals(vector.getObject(i), decoded.getObject(i)));
          }
        }
      }
    }
  }

  @Test
  public void testEncodeUnion() {
    // Create a new value vector
    try (final UnionVector vector = new UnionVector("vector", allocator, null);
        final UnionVector dictionaryVector = new UnionVector("dict", allocator, null);) {

      final NullableUInt4Holder uintHolder1 = new NullableUInt4Holder();
      uintHolder1.value = 10;
      uintHolder1.isSet = 1;

      final NullableIntHolder intHolder1 = new NullableIntHolder();
      intHolder1.value = 10;
      intHolder1.isSet = 1;

      final NullableIntHolder intHolder2 = new NullableIntHolder();
      intHolder2.value = 20;
      intHolder2.isSet = 1;

      //write data
      vector.setType(0, Types.MinorType.UINT4);
      vector.setSafe(0, uintHolder1);

      vector.setType(1, Types.MinorType.INT);
      vector.setSafe(1, intHolder1);

      vector.setType(2, Types.MinorType.INT);
      vector.setSafe(2, intHolder1);

      vector.setType(3, Types.MinorType.INT);
      vector.setSafe(3, intHolder2);

      vector.setType(4, Types.MinorType.INT);
      vector.setSafe(4, intHolder2);

      vector.setValueCount(5);

      //write dictionary
      dictionaryVector.setType(0, Types.MinorType.UINT4);
      dictionaryVector.setSafe(0, uintHolder1);

      dictionaryVector.setType(1, Types.MinorType.INT);
      dictionaryVector.setSafe(1, intHolder1);

      dictionaryVector.setType(2, Types.MinorType.INT);
      dictionaryVector.setSafe(2, intHolder2);

      dictionaryVector.setValueCount(3);

      Dictionary dictionary = new Dictionary(dictionaryVector, new DictionaryEncoding(1L, false, null));

      try (final ValueVector encoded = DictionaryEncoder.encode(vector, dictionary)) {
        // verify indices
        assertEquals(IntVector.class, encoded.getClass());

        IntVector index = ((IntVector)encoded);
        assertEquals(5, index.getValueCount());
        assertEquals(0, index.get(0));
        assertEquals(1, index.get(1));
        assertEquals(1, index.get(2));
        assertEquals(2, index.get(3));
        assertEquals(2, index.get(4));

        // now run through the decoder and verify we get the original back
        try (ValueVector decoded = DictionaryEncoder.decode(encoded, dictionary)) {
          assertEquals(vector.getClass(), decoded.getClass());
          assertEquals(vector.getValueCount(), decoded.getValueCount());
          for (int i = 0; i < 5; i++) {
            assertEquals(vector.getObject(i), decoded.getObject(i));
          }
        }
      }
    }
  }

  @Test
  public void testIntEquals() {
    //test Int
    try (final IntVector vector1 = new IntVector("", allocator);
        final IntVector vector2 = new IntVector("", allocator)) {

      Dictionary dict1 = new Dictionary(vector1, new DictionaryEncoding(1L, false, null));
      Dictionary dict2 = new Dictionary(vector2, new DictionaryEncoding(1L, false, null));

      vector1.allocateNew(3);
      vector1.setValueCount(3);
      vector2.allocateNew(3);
      vector2.setValueCount(3);

      vector1.setSafe(0, 1);
      vector1.setSafe(1, 2);
      vector1.setSafe(2, 3);

      vector2.setSafe(0, 1);
      vector2.setSafe(1, 2);
      vector2.setSafe(2, 0);

      assertFalse(dict1.equals(dict2));

      vector2.setSafe(2, 3);
      assertTrue(dict1.equals(dict2));
    }
  }

  @Test
  public void testVarcharEquals() {
    try (final VarCharVector vector1 = new VarCharVector("", allocator);
        final VarCharVector vector2 = new VarCharVector("", allocator)) {

      Dictionary dict1 = new Dictionary(vector1, new DictionaryEncoding(1L, false, null));
      Dictionary dict2 = new Dictionary(vector2, new DictionaryEncoding(1L, false, null));

      vector1.allocateNew();
      vector1.setValueCount(3);
      vector2.allocateNew();
      vector2.setValueCount(3);

      // set some values
      vector1.setSafe(0, zero, 0, zero.length);
      vector1.setSafe(1, one, 0, one.length);
      vector1.setSafe(2, two, 0, two.length);

      vector2.setSafe(0, zero, 0, zero.length);
      vector2.setSafe(1, one, 0, one.length);
      vector2.setSafe(2, one, 0, one.length);

      assertFalse(dict1.equals(dict2));

      vector2.setSafe(2, two, 0, two.length);
      assertTrue(dict1.equals(dict2));
    }
  }

  @Test
  public void testVarBinaryEquals() {
    try (final VarBinaryVector vector1 = new VarBinaryVector("", allocator);
        final VarBinaryVector vector2 = new VarBinaryVector("", allocator)) {

      Dictionary dict1 = new Dictionary(vector1, new DictionaryEncoding(1L, false, null));
      Dictionary dict2 = new Dictionary(vector2, new DictionaryEncoding(1L, false, null));

      vector1.allocateNew();
      vector1.setValueCount(3);
      vector2.allocateNew();
      vector2.setValueCount(3);

      // set some values
      vector1.setSafe(0, zero, 0, zero.length);
      vector1.setSafe(1, one, 0, one.length);
      vector1.setSafe(2, two, 0, two.length);

      vector2.setSafe(0, zero, 0, zero.length);
      vector2.setSafe(1, one, 0, one.length);
      vector2.setSafe(2, one, 0, one.length);

      assertFalse(dict1.equals(dict2));

      vector2.setSafe(2, two, 0, two.length);
      assertTrue(dict1.equals(dict2));
    }
  }

  @Test
  public void testListEquals() {
    try (final ListVector vector1 = ListVector.empty("", allocator);
        final ListVector vector2 = ListVector.empty("", allocator);) {

      Dictionary dict1 = new Dictionary(vector1, new DictionaryEncoding(1L, false, null));
      Dictionary dict2 = new Dictionary(vector2, new DictionaryEncoding(1L, false, null));

      UnionListWriter writer1 = vector1.getWriter();
      writer1.allocate();

      //set some values
      writeListVector(writer1, new int[] {1, 2});
      writeListVector(writer1, new int[] {3, 4});
      writeListVector(writer1, new int[] {5, 6});
      writer1.setValueCount(3);

      UnionListWriter writer2 = vector2.getWriter();
      writer2.allocate();

      //set some values
      writeListVector(writer2, new int[] {1, 2});
      writeListVector(writer2, new int[] {3, 4});
      writeListVector(writer2, new int[] {5, 6});
      writer2.setValueCount(3);

      assertTrue(dict1.equals(dict2));
    }
  }

  @Test
  public void testStructEquals() {
    try (final StructVector vector1 = StructVector.empty("", allocator);
        final StructVector vector2 = StructVector.empty("", allocator);) {
      vector1.addOrGet("f0", FieldType.nullable(new ArrowType.Int(32, true)), IntVector.class);
      vector1.addOrGet("f1", FieldType.nullable(new ArrowType.Int(64, true)), BigIntVector.class);
      vector2.addOrGet("f0", FieldType.nullable(new ArrowType.Int(32, true)), IntVector.class);
      vector2.addOrGet("f1", FieldType.nullable(new ArrowType.Int(64, true)), BigIntVector.class);

      Dictionary dict1 = new Dictionary(vector1, new DictionaryEncoding(1L, false, null));
      Dictionary dict2 = new Dictionary(vector2, new DictionaryEncoding(1L, false, null));

      NullableStructWriter writer1 = vector1.getWriter();
      writer1.allocate();

      writeStructVector(writer1, 1, 10L);
      writeStructVector(writer1, 2, 20L);
      writer1.setValueCount(2);

      NullableStructWriter writer2 = vector2.getWriter();
      writer2.allocate();

      writeStructVector(writer2, 1, 10L);
      writeStructVector(writer2, 2, 20L);
      writer2.setValueCount(2);

      assertTrue(dict1.equals(dict2));
    }
  }

  @Test
  public void testUnionEquals() {
    try (final UnionVector vector1 = new UnionVector("", allocator, null);
        final UnionVector vector2 = new UnionVector("", allocator, null);) {

      final NullableUInt4Holder uInt4Holder = new NullableUInt4Holder();
      uInt4Holder.value = 10;
      uInt4Holder.isSet = 1;

      final NullableIntHolder intHolder = new NullableIntHolder();
      uInt4Holder.value = 20;
      uInt4Holder.isSet = 1;

      vector1.setType(0, Types.MinorType.UINT4);
      vector1.setSafe(0, uInt4Holder);

      vector1.setType(2, Types.MinorType.INT);
      vector1.setSafe(2, intHolder);
      vector1.setValueCount(3);

      vector2.setType(0, Types.MinorType.UINT4);
      vector2.setSafe(0, uInt4Holder);

      vector2.setType(2, Types.MinorType.INT);
      vector2.setSafe(2, intHolder);
      vector2.setValueCount(3);

      Dictionary dict1 = new Dictionary(vector1, new DictionaryEncoding(1L, false, null));
      Dictionary dict2 = new Dictionary(vector2, new DictionaryEncoding(1L, false, null));

      assertTrue(dict1.equals(dict2));
    }
  }

  private void writeStructVector(NullableStructWriter writer, int value1, long value2) {
    writer.start();
    writer.integer("f0").writeInt(value1);
    writer.bigInt("f1").writeBigInt(value2);
    writer.end();
  }

  private void writeListVector(UnionListWriter writer, int[] values) {
    writer.startList();
    for (int v: values) {
      writer.integer().writeInt(v);
    }
    writer.endList();
  }
}
