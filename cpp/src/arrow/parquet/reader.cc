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

#include "arrow/parquet/reader.h"

#include <queue>

#include "arrow/parquet/schema.h"
#include "arrow/parquet/utils.h"
#include "arrow/schema.h"
#include "arrow/types/primitive.h"
#include "arrow/util/status.h"

using parquet::ColumnReader;
using parquet::TypedColumnReader;

namespace arrow {
namespace parquet {

class FileReader::Impl {
 public:
  Impl(MemoryPool* pool, std::unique_ptr<::parquet::ParquetFileReader> reader);
  virtual ~Impl() {}

  Status GetFlatColumn(int i, std::unique_ptr<FlatColumnReader>* out);
  Status ReadFlatColumn(int i, std::shared_ptr<Array>* out);

 private:
  MemoryPool* pool_;
  std::unique_ptr<::parquet::ParquetFileReader> reader_;
};

class FlatColumnReader::Impl {
 public:
  Impl(MemoryPool* pool, const ::parquet::ColumnDescriptor* descr,
      ::parquet::ParquetFileReader* reader, int column_index);
  virtual ~Impl() {}

  Status NextBatch(int batch_size, std::shared_ptr<Array>* out);
  template <typename ArrowType, typename ParquetType, typename CType>
  Status TypedReadBatch(int batch_size, std::shared_ptr<Array>* out);

 private:
  void NextRowGroup();

  MemoryPool* pool_;
  const ::parquet::ColumnDescriptor* descr_;
  ::parquet::ParquetFileReader* reader_;
  int column_index_;
  int next_row_group_;
  std::shared_ptr<ColumnReader> column_reader_;
  std::shared_ptr<Field> field_;

  PoolBuffer values_buffer_;
  PoolBuffer def_levels_buffer_;
  PoolBuffer rep_levels_buffer_;
};

FileReader::Impl::Impl(
    MemoryPool* pool, std::unique_ptr<::parquet::ParquetFileReader> reader)
    : pool_(pool), reader_(std::move(reader)) {}

Status FileReader::Impl::GetFlatColumn(int i, std::unique_ptr<FlatColumnReader>* out) {
  std::unique_ptr<FlatColumnReader::Impl> impl(
      new FlatColumnReader::Impl(pool_, reader_->descr()->Column(i), reader_.get(), i));
  *out = std::unique_ptr<FlatColumnReader>(new FlatColumnReader(std::move(impl)));
  return Status::OK();
}

Status FileReader::Impl::ReadFlatColumn(int i, std::shared_ptr<Array>* out) {
  std::unique_ptr<FlatColumnReader> flat_column_reader;
  RETURN_NOT_OK(GetFlatColumn(i, &flat_column_reader));
  return flat_column_reader->NextBatch(reader_->num_rows(), out);
}

FileReader::FileReader(
    MemoryPool* pool, std::unique_ptr<::parquet::ParquetFileReader> reader)
    : impl_(new FileReader::Impl(pool, std::move(reader))) {}

FileReader::~FileReader() {}

Status FileReader::GetFlatColumn(int i, std::unique_ptr<FlatColumnReader>* out) {
  return impl_->GetFlatColumn(i, out);
}

Status FileReader::ReadFlatColumn(int i, std::shared_ptr<Array>* out) {
  return impl_->ReadFlatColumn(i, out);
}

FlatColumnReader::Impl::Impl(MemoryPool* pool, const ::parquet::ColumnDescriptor* descr,
    ::parquet::ParquetFileReader* reader, int column_index)
    : pool_(pool),
      descr_(descr),
      reader_(reader),
      column_index_(column_index),
      next_row_group_(0),
      values_buffer_(pool),
      def_levels_buffer_(pool),
      rep_levels_buffer_(pool) {
  NodeToField(descr_->schema_node(), &field_);
  NextRowGroup();
}

template <typename ArrowType, typename ParquetType, typename CType>
Status FlatColumnReader::Impl::TypedReadBatch(
    int batch_size, std::shared_ptr<Array>* out) {
  int values_to_read = batch_size;
  NumericBuilder<ArrowType> builder(pool_, field_->type);
  while ((values_to_read > 0) && column_reader_) {
    values_buffer_.Resize(values_to_read * sizeof(CType));
    if (descr_->max_definition_level() > 0) {
      def_levels_buffer_.Resize(values_to_read * sizeof(int16_t));
    }
    if (descr_->max_repetition_level() > 0) {
      rep_levels_buffer_.Resize(values_to_read * sizeof(int16_t));
    }
    auto reader = dynamic_cast<TypedColumnReader<ParquetType>*>(column_reader_.get());
    int64_t values_read;
    CType* values = reinterpret_cast<CType*>(values_buffer_.mutable_data());
    PARQUET_CATCH_NOT_OK(
        values_to_read -= reader->ReadBatch(values_to_read,
            reinterpret_cast<int16_t*>(def_levels_buffer_.mutable_data()),
            reinterpret_cast<int16_t*>(rep_levels_buffer_.mutable_data()), values,
            &values_read));
    if (descr_->max_definition_level() == 0) {
      RETURN_NOT_OK(builder.Append(values, values_read));
    } else {
      return Status::NotImplemented("no support for definition levels yet");
    }
    if (!column_reader_->HasNext()) { NextRowGroup(); }
  }
  *out = builder.Finish();
  return Status::OK();
}

#define TYPED_BATCH_CASE(ENUM, ArrowType, ParquetType, CType)              \
  case Type::ENUM:                                                         \
    return TypedReadBatch<ArrowType, ParquetType, CType>(batch_size, out); \
    break;

Status FlatColumnReader::Impl::NextBatch(int batch_size, std::shared_ptr<Array>* out) {
  if (!column_reader_) {
    // Exhausted all row groups.
    *out = nullptr;
    return Status::OK();
  }

  if (descr_->max_repetition_level() > 0) {
    return Status::NotImplemented("no support for repetition yet");
  }

  switch (field_->type->type) {
    TYPED_BATCH_CASE(INT32, Int32Type, ::parquet::Int32Type, int32_t)
    TYPED_BATCH_CASE(INT64, Int64Type, ::parquet::Int64Type, int64_t)
    TYPED_BATCH_CASE(FLOAT, FloatType, ::parquet::FloatType, float)
    TYPED_BATCH_CASE(DOUBLE, DoubleType, ::parquet::DoubleType, double)
    default:
      return Status::NotImplemented(field_->type->ToString());
  }
}

void FlatColumnReader::Impl::NextRowGroup() {
  if (next_row_group_ < reader_->num_row_groups()) {
    column_reader_ = reader_->RowGroup(next_row_group_)->Column(column_index_);
    next_row_group_++;
  } else {
    column_reader_ = nullptr;
  }
}

FlatColumnReader::FlatColumnReader(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

FlatColumnReader::~FlatColumnReader() {}

Status FlatColumnReader::NextBatch(int batch_size, std::shared_ptr<Array>* out) {
  return impl_->NextBatch(batch_size, out);
}

}  // namespace parquet
}  // namespace arrow
