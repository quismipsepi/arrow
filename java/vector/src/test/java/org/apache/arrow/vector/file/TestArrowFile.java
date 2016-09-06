/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.arrow.vector.file;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.memory.RootAllocator;
import org.apache.arrow.vector.FieldVector;
import org.apache.arrow.vector.ValueVector.Accessor;
import org.apache.arrow.vector.VectorLoader;
import org.apache.arrow.vector.VectorUnloader;
import org.apache.arrow.vector.complex.MapVector;
import org.apache.arrow.vector.complex.NullableMapVector;
import org.apache.arrow.vector.complex.impl.ComplexWriterImpl;
import org.apache.arrow.vector.complex.impl.SingleMapReaderImpl;
import org.apache.arrow.vector.complex.reader.BaseReader.MapReader;
import org.apache.arrow.vector.complex.writer.BaseWriter.ComplexWriter;
import org.apache.arrow.vector.complex.writer.BaseWriter.ListWriter;
import org.apache.arrow.vector.complex.writer.BaseWriter.MapWriter;
import org.apache.arrow.vector.complex.writer.BigIntWriter;
import org.apache.arrow.vector.complex.writer.IntWriter;
import org.apache.arrow.vector.schema.ArrowBuffer;
import org.apache.arrow.vector.schema.ArrowRecordBatch;
import org.apache.arrow.vector.types.Types.MinorType;
import org.apache.arrow.vector.types.pojo.Schema;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.netty.buffer.ArrowBuf;

public class TestArrowFile {
  private static final Logger LOGGER = LoggerFactory.getLogger(TestArrowFile.class);
  private static final int COUNT = 10;
  private BufferAllocator allocator;

  @Before
  public void init() {
    allocator = new RootAllocator(Integer.MAX_VALUE);
  }

  @After
  public void tearDown() {
    allocator.close();
  }

  @Test
  public void testWrite() throws IOException {
    File file = new File("target/mytest_write.arrow");
    int count = COUNT;
    try (
        BufferAllocator vectorAllocator = allocator.newChildAllocator("original vectors", 0, Integer.MAX_VALUE);
        MapVector parent = new MapVector("parent", vectorAllocator, null)) {
      writeData(count, parent);
      write(parent.getChild("root"), file);
    }
  }

  @Test
  public void testWriteComplex() throws IOException {
    File file = new File("target/mytest_write_complex.arrow");
    int count = COUNT;
    try (
        BufferAllocator vectorAllocator = allocator.newChildAllocator("original vectors", 0, Integer.MAX_VALUE);
        NullableMapVector parent = new NullableMapVector("parent", vectorAllocator, null)) {
      writeComplexData(count, parent);
      validateComplexContent(count, parent);
      write(parent.getChild("root"), file);
    }
  }

  private void writeComplexData(int count, MapVector parent) {
    ArrowBuf varchar = allocator.buffer(3);
    varchar.readerIndex(0);
    varchar.setByte(0, 'a');
    varchar.setByte(1, 'b');
    varchar.setByte(2, 'c');
    varchar.writerIndex(3);
    ComplexWriter writer = new ComplexWriterImpl("root", parent);
    MapWriter rootWriter = writer.rootAsMap();
    IntWriter intWriter = rootWriter.integer("int");
    BigIntWriter bigIntWriter = rootWriter.bigInt("bigInt");
    ListWriter listWriter = rootWriter.list("list");
    MapWriter mapWriter = rootWriter.map("map");
    for (int i = 0; i < count; i++) {
      intWriter.setPosition(i);
      intWriter.writeInt(i);
      bigIntWriter.setPosition(i);
      bigIntWriter.writeBigInt(i);
      listWriter.setPosition(i);
      listWriter.startList();
      for (int j = 0; j < i % 3; j++) {
        listWriter.varChar().writeVarChar(0, 3, varchar);
      }
      listWriter.endList();
      mapWriter.setPosition(i);
      mapWriter.start();
      mapWriter.timeStamp("timestamp").writeTimeStamp(i);
      mapWriter.end();
    }
    writer.setValueCount(count);
    varchar.release();
  }


  private void writeData(int count, MapVector parent) {
    ComplexWriter writer = new ComplexWriterImpl("root", parent);
    MapWriter rootWriter = writer.rootAsMap();
    IntWriter intWriter = rootWriter.integer("int");
    BigIntWriter bigIntWriter = rootWriter.bigInt("bigInt");
    for (int i = 0; i < count; i++) {
      intWriter.setPosition(i);
      intWriter.writeInt(i);
      bigIntWriter.setPosition(i);
      bigIntWriter.writeBigInt(i);
    }
    writer.setValueCount(count);
  }

  @Test
  public void testWriteRead() throws IOException {
    File file = new File("target/mytest.arrow");
    int count = COUNT;

    // write
    try (
        BufferAllocator originalVectorAllocator = allocator.newChildAllocator("original vectors", 0, Integer.MAX_VALUE);
        MapVector parent = new MapVector("parent", originalVectorAllocator, null)) {
      writeData(count, parent);
      write(parent.getChild("root"), file);
    }

    // read
    try (
        BufferAllocator readerAllocator = allocator.newChildAllocator("reader", 0, Integer.MAX_VALUE);
        FileInputStream fileInputStream = new FileInputStream(file);
        ArrowReader arrowReader = new ArrowReader(fileInputStream.getChannel(), readerAllocator);
        BufferAllocator vectorAllocator = allocator.newChildAllocator("final vectors", 0, Integer.MAX_VALUE);
        MapVector parent = new MapVector("parent", vectorAllocator, null)
        ) {
      ArrowFooter footer = arrowReader.readFooter();
      Schema schema = footer.getSchema();
      LOGGER.debug("reading schema: " + schema);

      // initialize vectors

      NullableMapVector root = parent.addOrGet("root", MinorType.MAP, NullableMapVector.class);

      VectorLoader vectorLoader = new VectorLoader(schema, root);

      List<ArrowBlock> recordBatches = footer.getRecordBatches();
      for (ArrowBlock rbBlock : recordBatches) {
        Assert.assertEquals(0, rbBlock.getOffset() % 8);
        Assert.assertEquals(0, rbBlock.getMetadataLength() % 8);
        try (ArrowRecordBatch recordBatch = arrowReader.readRecordBatch(rbBlock)) {
          List<ArrowBuffer> buffersLayout = recordBatch.getBuffersLayout();
          for (ArrowBuffer arrowBuffer : buffersLayout) {
            Assert.assertEquals(0, arrowBuffer.getOffset() % 8);
          }
          vectorLoader.load(recordBatch);
        }

        validateContent(count, parent);
      }
    }
  }

  private void validateContent(int count, MapVector parent) {
    MapReader rootReader = new SingleMapReaderImpl(parent).reader("root");
    for (int i = 0; i < count; i++) {
      rootReader.setPosition(i);
      Assert.assertEquals(i, rootReader.reader("int").readInteger().intValue());
      Assert.assertEquals(i, rootReader.reader("bigInt").readLong().longValue());
    }
  }

  @Test
  public void testWriteReadComplex() throws IOException {
    File file = new File("target/mytest_complex.arrow");
    int count = COUNT;

    // write
    try (
        BufferAllocator originalVectorAllocator = allocator.newChildAllocator("original vectors", 0, Integer.MAX_VALUE);
        MapVector parent = new MapVector("parent", originalVectorAllocator, null)) {
      writeComplexData(count, parent);
      write(parent.getChild("root"), file);
    }

    // read
    try (
        BufferAllocator readerAllocator = allocator.newChildAllocator("reader", 0, Integer.MAX_VALUE);
        FileInputStream fileInputStream = new FileInputStream(file);
        ArrowReader arrowReader = new ArrowReader(fileInputStream.getChannel(), readerAllocator);
        BufferAllocator vectorAllocator = allocator.newChildAllocator("final vectors", 0, Integer.MAX_VALUE);
        NullableMapVector parent = new NullableMapVector("parent", vectorAllocator, null)
        ) {
      ArrowFooter footer = arrowReader.readFooter();
      Schema schema = footer.getSchema();
      LOGGER.debug("reading schema: " + schema);

      // initialize vectors

      NullableMapVector root = parent.addOrGet("root", MinorType.MAP, NullableMapVector.class);
      VectorLoader vectorLoader = new VectorLoader(schema, root);

      List<ArrowBlock> recordBatches = footer.getRecordBatches();
      for (ArrowBlock rbBlock : recordBatches) {
        try (ArrowRecordBatch recordBatch = arrowReader.readRecordBatch(rbBlock)) {
          vectorLoader.load(recordBatch);
        }
        validateComplexContent(count, parent);
      }
    }
  }

  public void printVectors(List<FieldVector> vectors) {
    for (FieldVector vector : vectors) {
      LOGGER.debug(vector.getField().getName());
      Accessor accessor = vector.getAccessor();
      int valueCount = accessor.getValueCount();
      for (int i = 0; i < valueCount; i++) {
        LOGGER.debug(String.valueOf(accessor.getObject(i)));
      }
    }
  }

  private void validateComplexContent(int count, NullableMapVector parent) {
    printVectors(parent.getChildrenFromFields());

    MapReader rootReader = new SingleMapReaderImpl(parent).reader("root");
    for (int i = 0; i < count; i++) {
      rootReader.setPosition(i);
      Assert.assertEquals(i, rootReader.reader("int").readInteger().intValue());
      Assert.assertEquals(i, rootReader.reader("bigInt").readLong().longValue());
      Assert.assertEquals(i % 3, rootReader.reader("list").size());
      Assert.assertEquals(i, rootReader.reader("map").reader("timestamp").readDateTime().getMillis() % COUNT);
    }
  }

  private void write(FieldVector parent, File file) throws FileNotFoundException, IOException {
    VectorUnloader vectorUnloader = new VectorUnloader(parent);
    Schema schema = vectorUnloader.getSchema();
    LOGGER.debug("writing schema: " + schema);
    try (
        FileOutputStream fileOutputStream = new FileOutputStream(file);
        ArrowWriter arrowWriter = new ArrowWriter(fileOutputStream.getChannel(), schema);
        ArrowRecordBatch recordBatch = vectorUnloader.getRecordBatch();
            ) {
      arrowWriter.writeRecordBatch(recordBatch);
    }
  }

  @Test
  public void testWriteReadMultipleRBs() throws IOException {
    File file = new File("target/mytest_multiple.arrow");
    int count = COUNT;

    // write
    try (
        BufferAllocator originalVectorAllocator = allocator.newChildAllocator("original vectors", 0, Integer.MAX_VALUE);
        MapVector parent = new MapVector("parent", originalVectorAllocator, null);
        FileOutputStream fileOutputStream = new FileOutputStream(file);) {
      writeData(count, parent);
      VectorUnloader vectorUnloader = new VectorUnloader(parent.getChild("root"));
      Schema schema = vectorUnloader.getSchema();
      Assert.assertEquals(2, schema.getFields().size());
      try (ArrowWriter arrowWriter = new ArrowWriter(fileOutputStream.getChannel(), schema);) {
        try (ArrowRecordBatch recordBatch = vectorUnloader.getRecordBatch()) {
          arrowWriter.writeRecordBatch(recordBatch);
        }
        parent.allocateNew();
        writeData(count, parent);
        try (ArrowRecordBatch recordBatch = vectorUnloader.getRecordBatch()) {
          arrowWriter.writeRecordBatch(recordBatch);
        }
      }
    }

    // read
    try (
        BufferAllocator readerAllocator = allocator.newChildAllocator("reader", 0, Integer.MAX_VALUE);
        FileInputStream fileInputStream = new FileInputStream(file);
        ArrowReader arrowReader = new ArrowReader(fileInputStream.getChannel(), readerAllocator);
        BufferAllocator vectorAllocator = allocator.newChildAllocator("final vectors", 0, Integer.MAX_VALUE);
        MapVector parent = new MapVector("parent", vectorAllocator, null);
        ) {
      ArrowFooter footer = arrowReader.readFooter();
      Schema schema = footer.getSchema();
      LOGGER.debug("reading schema: " + schema);
      NullableMapVector root = parent.addOrGet("root", MinorType.MAP, NullableMapVector.class);
      VectorLoader vectorLoader = new VectorLoader(schema, root);
      List<ArrowBlock> recordBatches = footer.getRecordBatches();
      Assert.assertEquals(2, recordBatches.size());
      for (ArrowBlock rbBlock : recordBatches) {
        Assert.assertEquals(0, rbBlock.getOffset() % 8);
        Assert.assertEquals(0, rbBlock.getMetadataLength() % 8);
        try (ArrowRecordBatch recordBatch = arrowReader.readRecordBatch(rbBlock)) {
          List<ArrowBuffer> buffersLayout = recordBatch.getBuffersLayout();
          for (ArrowBuffer arrowBuffer : buffersLayout) {
            Assert.assertEquals(0, arrowBuffer.getOffset() % 8);
          }
          vectorLoader.load(recordBatch);
          validateContent(count, parent);
        }
      }
    }
  }

}
