// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef ARROW_PARQUET_SCHEMA_H
#define ARROW_PARQUET_SCHEMA_H

#include <memory>

#include "parquet/api/schema.h"
#include "parquet/api/writer.h"

#include "arrow/schema.h"
#include "arrow/type.h"
#include "arrow/util/visibility.h"

namespace arrow {

class Status;

namespace parquet {

Status ARROW_EXPORT NodeToField(
    const ::parquet::schema::NodePtr& node, std::shared_ptr<Field>* out);

Status ARROW_EXPORT FromParquetSchema(
    const ::parquet::SchemaDescriptor* parquet_schema, std::shared_ptr<Schema>* out);

Status ARROW_EXPORT FieldToNode(const std::shared_ptr<Field>& field,
    const ::parquet::WriterProperties& properties, ::parquet::schema::NodePtr* out);

Status ARROW_EXPORT ToParquetSchema(const Schema* arrow_schema,
    const ::parquet::WriterProperties& properties,
    std::shared_ptr<::parquet::SchemaDescriptor>* out);

}  // namespace parquet

}  // namespace arrow

#endif  // ARROW_PARQUET_SCHEMA_H
