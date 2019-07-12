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

package org.apache.arrow.algorithm.sort;

import org.apache.arrow.vector.BigIntVector;
import org.apache.arrow.vector.Float4Vector;
import org.apache.arrow.vector.Float8Vector;
import org.apache.arrow.vector.IntVector;
import org.apache.arrow.vector.SmallIntVector;
import org.apache.arrow.vector.TinyIntVector;
import org.apache.arrow.vector.VarCharVector;
import org.apache.arrow.vector.holders.NullableVarCharHolder;

/**
 * Default comparator implementations for different types of vectors.
 */
public class DefaultVectorComparators {

  /**
   * Default comparator for bytes.
   * The comparison is based on values, with null comes first.
   */
  public static class ByteComparator extends VectorValueComparator<TinyIntVector> {

    public ByteComparator() {
      super(Byte.SIZE / 8);
    }

    @Override
    public int compareNotNull(int index1, int index2) {
      byte value1 = vector1.get(index1);
      byte value2 = vector2.get(index2);
      return value1 - value2;
    }
  }

  /**
   * Default comparator for short integers.
   * The comparison is based on values, with null comes first.
   */
  public static class ShortComparator extends VectorValueComparator<SmallIntVector> {

    public ShortComparator() {
      super(Short.SIZE / 8);
    }

    @Override
    public int compareNotNull(int index1, int index2) {
      short value1 = vector1.get(index1);
      short value2 = vector2.get(index2);
      return value1 - value2;
    }
  }

  /**
   * Default comparator for 32-bit integers.
   * The comparison is based on int values, with null comes first.
   */
  public static class IntComparator extends VectorValueComparator<IntVector> {

    public IntComparator() {
      super(Integer.SIZE / 8);
    }

    @Override
    public int compareNotNull(int index1, int index2) {
      int value1 = vector1.get(index1);
      int value2 = vector2.get(index2);
      return value1 - value2;
    }
  }

  /**
   * Default comparator for long integers.
   * The comparison is based on values, with null comes first.
   */
  public static class LongComparator extends VectorValueComparator<BigIntVector> {

    public LongComparator() {
      super(Long.SIZE / 8);
    }

    @Override
    public int compareNotNull(int index1, int index2) {
      long value1 = vector1.get(index1);
      long value2 = vector2.get(index2);

      return Long.signum(value1 - value2);
    }
  }

  /**
   * Default comparator for float type.
   * The comparison is based on values, with null comes first.
   */
  public static class Float4Comparator extends VectorValueComparator<Float4Vector> {

    public Float4Comparator() {
      super(Float.SIZE / 8);
    }

    @Override
    public int compareNotNull(int index1, int index2) {
      float value1 = vector1.get(index1);
      float value2 = vector2.get(index2);

      boolean isNan1 = Float.isNaN(value1);
      boolean isNan2 = Float.isNaN(value2);
      if (isNan1 || isNan2) {
        if (isNan1 && isNan2) {
          return 0;
        } else if (isNan1) {
          // nan is greater than any normal value
          return 1;
        } else {
          return -1;
        }
      }

      return (int) Math.signum(value1 - value2);
    }
  }

  /**
   * Default comparator for double type.
   * The comparison is based on values, with null comes first.
   */
  public static class Float8Comparator extends VectorValueComparator<Float8Vector> {

    public Float8Comparator() {
      super(Double.SIZE / 8);
    }

    @Override
    public int compareNotNull(int index1, int index2) {
      double value1 = vector1.get(index1);
      double value2 = vector2.get(index2);

      boolean isNan1 = Double.isNaN(value1);
      boolean isNan2 = Double.isNaN(value2);
      if (isNan1 || isNan2) {
        if (isNan1 && isNan2) {
          return 0;
        } else if (isNan1) {
          // nan is greater than any normal value
          return 1;
        } else {
          return -1;
        }
      }

      return (int) Math.signum(value1 - value2);
    }
  }

  /**
   * Default comparator for varchars.
   * The comparison is in lexicographic order, with null comes first.
   */
  public static class VarCharComparator extends VectorValueComparator<VarCharVector> {

    private NullableVarCharHolder holder1 = new NullableVarCharHolder();
    private NullableVarCharHolder holder2 = new NullableVarCharHolder();

    @Override
    public int compareNotNull(int index1, int index2) {
      vector1.get(index1, holder1);
      vector2.get(index2, holder2);

      int length1 = holder1.end - holder1.start;
      int length2 = holder2.end - holder2.start;

      int minLength = length1 < length2 ? length1 : length2;
      for (int i = 0; i < minLength; i++) {
        byte b1 = holder1.buffer.getByte(holder1.start + i);
        byte b2 = holder2.buffer.getByte(holder2.start + i);

        if (b1 != b2) {
          return b1 - b2;
        }
      }

      return length1 - length2;
    }
  }

  private DefaultVectorComparators() {
  }
}
