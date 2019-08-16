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

package org.apache.arrow.adapter.jdbc.consumer;

import java.sql.ResultSet;
import java.sql.SQLException;

import org.apache.arrow.vector.Float8Vector;
import org.apache.arrow.vector.complex.impl.Float8WriterImpl;
import org.apache.arrow.vector.complex.writer.Float8Writer;

/**
 * Consumer which consume double type values from {@link ResultSet}.
 * Write the data to {@link org.apache.arrow.vector.Float8Vector}.
 */
public class DoubleConsumer implements JdbcConsumer<Float8Vector> {

  private Float8Writer writer;
  private final int columnIndexInResultSet;

  /**
   * Instantiate a DoubleConsumer.
   */
  public DoubleConsumer(Float8Vector vector, int index) {
    this.writer = new Float8WriterImpl(vector);
    this.columnIndexInResultSet = index;
  }

  @Override
  public void consume(ResultSet resultSet) throws SQLException {
    double value = resultSet.getDouble(columnIndexInResultSet);
    if (!resultSet.wasNull()) {
      writer.writeFloat8(value);
    }
    writer.setPosition(writer.getPosition() + 1);
  }

  @Override
  public void resetValueVector(Float8Vector vector) {
    this.writer = new Float8WriterImpl(vector);
  }
}
