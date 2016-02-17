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
package org.apache.arrow.vector;

import io.netty.buffer.ArrowBuf;

import java.util.Iterator;

import org.apache.arrow.memory.BufferAllocator;
import org.apache.arrow.memory.OutOfMemoryException;
import org.apache.arrow.vector.complex.impl.NullReader;
import org.apache.arrow.vector.complex.reader.FieldReader;
import org.apache.arrow.vector.types.MaterializedField;
import org.apache.arrow.vector.types.Types;
import org.apache.arrow.vector.types.Types.MinorType;
import org.apache.arrow.vector.util.TransferPair;

import com.google.common.collect.Iterators;

public class ZeroVector implements ValueVector {
  public final static ZeroVector INSTANCE = new ZeroVector();

  private final MaterializedField field = MaterializedField.create("[DEFAULT]", Types.required(MinorType.LATE));

  private final TransferPair defaultPair = new TransferPair() {
    @Override
    public void transfer() { }

    @Override
    public void splitAndTransfer(int startIndex, int length) { }

    @Override
    public ValueVector getTo() {
      return ZeroVector.this;
    }

    @Override
    public void copyValueSafe(int from, int to) { }
  };

  private final Accessor defaultAccessor = new Accessor() {
    @Override
    public Object getObject(int index) {
      return null;
    }

    @Override
    public int getValueCount() {
      return 0;
    }

    @Override
    public boolean isNull(int index) {
      return true;
    }
  };

  private final Mutator defaultMutator = new Mutator() {
    @Override
    public void setValueCount(int valueCount) { }

    @Override
    public void reset() { }

    @Override
    public void generateTestData(int values) { }
  };

  public ZeroVector() { }

  @Override
  public void close() { }

  @Override
  public void clear() { }

  @Override
  public MaterializedField getField() {
    return field;
  }

  @Override
  public TransferPair getTransferPair(BufferAllocator allocator) {
    return defaultPair;
  }

//  @Override
//  public UserBitShared.SerializedField getMetadata() {
//    return getField()
//        .getAsBuilder()
//        .setBufferLength(getBufferSize())
//        .setValueCount(getAccessor().getValueCount())
//        .build();
//  }

  @Override
  public Iterator iterator() {
    return Iterators.emptyIterator();
  }

  @Override
  public int getBufferSize() {
    return 0;
  }

  @Override
  public int getBufferSizeFor(final int valueCount) {
    return 0;
  }

  @Override
  public ArrowBuf[] getBuffers(boolean clear) {
    return new ArrowBuf[0];
  }

  @Override
  public void allocateNew() throws OutOfMemoryException {
    allocateNewSafe();
  }

  @Override
  public boolean allocateNewSafe() {
    return true;
  }

  @Override
  public BufferAllocator getAllocator() {
    throw new UnsupportedOperationException("Tried to get allocator from ZeroVector");
  }

  @Override
  public void setInitialCapacity(int numRecords) { }

  @Override
  public int getValueCapacity() {
    return 0;
  }

  @Override
  public TransferPair getTransferPair(String ref, BufferAllocator allocator) {
    return defaultPair;
  }

  @Override
  public TransferPair makeTransferPair(ValueVector target) {
    return defaultPair;
  }

  @Override
  public Accessor getAccessor() {
    return defaultAccessor;
  }

  @Override
  public Mutator getMutator() {
    return defaultMutator;
  }

  @Override
  public FieldReader getReader() {
    return NullReader.INSTANCE;
  }

//  @Override
//  public void load(UserBitShared.SerializedField metadata, DrillBuf buffer) { }
}
