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

#include "arrow/ipc/adapter.h"

#include <cstdint>
#include <cstring>
#include <sstream>
#include <vector>

#include "arrow/array.h"
#include "arrow/buffer.h"
#include "arrow/io/interfaces.h"
#include "arrow/io/memory.h"
#include "arrow/ipc/Message_generated.h"
#include "arrow/ipc/metadata-internal.h"
#include "arrow/ipc/metadata.h"
#include "arrow/ipc/util.h"
#include "arrow/schema.h"
#include "arrow/status.h"
#include "arrow/table.h"
#include "arrow/type.h"
#include "arrow/type_fwd.h"
#include "arrow/util/bit-util.h"
#include "arrow/util/logging.h"

namespace arrow {

namespace flatbuf = org::apache::arrow::flatbuf;

namespace ipc {

// ----------------------------------------------------------------------
// Record batch write path

class RecordBatchWriter : public ArrayVisitor {
 public:
  RecordBatchWriter(const std::vector<std::shared_ptr<Array>>& columns, int32_t num_rows,
      int64_t buffer_start_offset, int max_recursion_depth)
      : columns_(columns),
        num_rows_(num_rows),
        max_recursion_depth_(max_recursion_depth),
        buffer_start_offset_(buffer_start_offset) {}

  Status VisitArray(const Array& arr) {
    if (max_recursion_depth_ <= 0) {
      return Status::Invalid("Max recursion depth reached");
    }
    // push back all common elements
    field_nodes_.push_back(flatbuf::FieldNode(arr.length(), arr.null_count()));
    if (arr.null_count() > 0) {
      buffers_.push_back(arr.null_bitmap());
    } else {
      // Push a dummy zero-length buffer, not to be copied
      buffers_.push_back(std::make_shared<Buffer>(nullptr, 0));
    }
    return arr.Accept(this);
  }

  Status Assemble(int64_t* body_length) {
    if (field_nodes_.size() > 0) {
      field_nodes_.clear();
      buffer_meta_.clear();
      buffers_.clear();
    }

    // Perform depth-first traversal of the row-batch
    for (size_t i = 0; i < columns_.size(); ++i) {
      RETURN_NOT_OK(VisitArray(*columns_[i].get()));
    }

    // The position for the start of a buffer relative to the passed frame of
    // reference. May be 0 or some other position in an address space
    int64_t offset = buffer_start_offset_;

    // Construct the buffer metadata for the record batch header
    for (size_t i = 0; i < buffers_.size(); ++i) {
      const Buffer* buffer = buffers_[i].get();
      int64_t size = 0;
      int64_t padding = 0;

      // The buffer might be null if we are handling zero row lengths.
      if (buffer) {
        size = buffer->size();
        padding = BitUtil::RoundUpToMultipleOf64(size) - size;
      }

      // TODO(wesm): We currently have no notion of shared memory page id's,
      // but we've included it in the metadata IDL for when we have it in the
      // future. Use page = -1 for now
      //
      // Note that page ids are a bespoke notion for Arrow and not a feature we
      // are using from any OS-level shared memory. The thought is that systems
      // may (in the future) associate integer page id's with physical memory
      // pages (according to whatever is the desired shared memory mechanism)
      buffer_meta_.push_back(flatbuf::Buffer(-1, offset, size + padding));
      offset += size + padding;
    }

    *body_length = offset - buffer_start_offset_;
    DCHECK(BitUtil::IsMultipleOf64(*body_length));

    return Status::OK();
  }

  Status WriteMetadata(
      int64_t body_length, io::OutputStream* dst, int32_t* metadata_length) {
    // Now that we have computed the locations of all of the buffers in shared
    // memory, the data header can be converted to a flatbuffer and written out
    //
    // Note: The memory written here is prefixed by the size of the flatbuffer
    // itself as an int32_t.
    std::shared_ptr<Buffer> metadata_fb;
    RETURN_NOT_OK(WriteRecordBatchMetadata(
        num_rows_, body_length, field_nodes_, buffer_meta_, &metadata_fb));

    // Need to write 4 bytes (metadata size), the metadata, plus padding to
    // fall on a 64-byte offset
    int64_t padded_metadata_length =
        BitUtil::RoundUpToMultipleOf64(metadata_fb->size() + 4);

    // The returned metadata size includes the length prefix, the flatbuffer,
    // plus padding
    *metadata_length = padded_metadata_length;

    // Write the flatbuffer size prefix
    int32_t flatbuffer_size = metadata_fb->size();
    RETURN_NOT_OK(
        dst->Write(reinterpret_cast<const uint8_t*>(&flatbuffer_size), sizeof(int32_t)));

    // Write the flatbuffer
    RETURN_NOT_OK(dst->Write(metadata_fb->data(), metadata_fb->size()));

    // Write any padding
    int64_t padding = padded_metadata_length - metadata_fb->size() - 4;
    if (padding > 0) { RETURN_NOT_OK(dst->Write(kPaddingBytes, padding)); }

    return Status::OK();
  }

  Status Write(io::OutputStream* dst, int32_t* metadata_length, int64_t* body_length) {
    RETURN_NOT_OK(Assemble(body_length));

#ifndef NDEBUG
    int64_t start_position, current_position;
    RETURN_NOT_OK(dst->Tell(&start_position));
#endif

    RETURN_NOT_OK(WriteMetadata(*body_length, dst, metadata_length));

#ifndef NDEBUG
    RETURN_NOT_OK(dst->Tell(&current_position));
    DCHECK(BitUtil::IsMultipleOf8(current_position));
#endif

    // Now write the buffers
    for (size_t i = 0; i < buffers_.size(); ++i) {
      const Buffer* buffer = buffers_[i].get();
      int64_t size = 0;
      int64_t padding = 0;

      // The buffer might be null if we are handling zero row lengths.
      if (buffer) {
        size = buffer->size();
        padding = BitUtil::RoundUpToMultipleOf64(size) - size;
      }

      if (size > 0) { RETURN_NOT_OK(dst->Write(buffer->data(), size)); }

      if (padding > 0) { RETURN_NOT_OK(dst->Write(kPaddingBytes, padding)); }
    }

#ifndef NDEBUG
    RETURN_NOT_OK(dst->Tell(&current_position));
    DCHECK(BitUtil::IsMultipleOf8(current_position));
#endif

    return Status::OK();
  }

  Status GetTotalSize(int64_t* size) {
    // emulates the behavior of Write without actually writing
    int32_t metadata_length;
    int64_t body_length;
    MockOutputStream dst;
    RETURN_NOT_OK(Write(&dst, &metadata_length, &body_length));
    *size = dst.GetExtentBytesWritten();
    return Status::OK();
  }

 private:
  Status Visit(const NullArray& array) override { return Status::NotImplemented("null"); }

  Status VisitPrimitive(const PrimitiveArray& array) {
    buffers_.push_back(array.data());
    return Status::OK();
  }

  Status VisitBinary(const BinaryArray& array) {
    buffers_.push_back(array.offsets());
    buffers_.push_back(array.data());
    return Status::OK();
  }

  Status Visit(const BooleanArray& array) override { return VisitPrimitive(array); }

  Status Visit(const Int8Array& array) override { return VisitPrimitive(array); }

  Status Visit(const Int16Array& array) override { return VisitPrimitive(array); }

  Status Visit(const Int32Array& array) override { return VisitPrimitive(array); }

  Status Visit(const Int64Array& array) override { return VisitPrimitive(array); }

  Status Visit(const UInt8Array& array) override { return VisitPrimitive(array); }

  Status Visit(const UInt16Array& array) override { return VisitPrimitive(array); }

  Status Visit(const UInt32Array& array) override { return VisitPrimitive(array); }

  Status Visit(const UInt64Array& array) override { return VisitPrimitive(array); }

  Status Visit(const HalfFloatArray& array) override { return VisitPrimitive(array); }

  Status Visit(const FloatArray& array) override { return VisitPrimitive(array); }

  Status Visit(const DoubleArray& array) override { return VisitPrimitive(array); }

  Status Visit(const StringArray& array) override { return VisitBinary(array); }

  Status Visit(const BinaryArray& array) override { return VisitBinary(array); }

  Status Visit(const DateArray& array) override { return VisitPrimitive(array); }

  Status Visit(const TimeArray& array) override { return VisitPrimitive(array); }

  Status Visit(const TimestampArray& array) override { return VisitPrimitive(array); }

  Status Visit(const IntervalArray& array) override {
    return Status::NotImplemented("interval");
  }

  Status Visit(const DecimalArray& array) override {
    return Status::NotImplemented("decimal");
  }

  Status Visit(const ListArray& array) override {
    buffers_.push_back(array.offsets());
    --max_recursion_depth_;
    RETURN_NOT_OK(VisitArray(*array.values().get()));
    ++max_recursion_depth_;
    return Status::OK();
  }

  Status Visit(const StructArray& array) override {
    --max_recursion_depth_;
    for (const auto& field : array.fields()) {
      RETURN_NOT_OK(VisitArray(*field.get()));
    }
    ++max_recursion_depth_;
    return Status::OK();
  }

  Status Visit(const UnionArray& array) override {
    return Status::NotImplemented("union");
  }

  // Do not copy this vector. Ownership must be retained elsewhere
  const std::vector<std::shared_ptr<Array>>& columns_;
  int32_t num_rows_;

  std::vector<flatbuf::FieldNode> field_nodes_;
  std::vector<flatbuf::Buffer> buffer_meta_;
  std::vector<std::shared_ptr<Buffer>> buffers_;

  int64_t max_recursion_depth_;
  int64_t buffer_start_offset_;
};

Status WriteRecordBatch(const std::vector<std::shared_ptr<Array>>& columns,
    int32_t num_rows, int64_t buffer_start_offset, io::OutputStream* dst,
    int32_t* metadata_length, int64_t* body_length, int max_recursion_depth) {
  DCHECK_GT(max_recursion_depth, 0);
  RecordBatchWriter serializer(
      columns, num_rows, buffer_start_offset, max_recursion_depth);
  return serializer.Write(dst, metadata_length, body_length);
}

Status GetRecordBatchSize(const RecordBatch* batch, int64_t* size) {
  RecordBatchWriter serializer(
      batch->columns(), batch->num_rows(), 0, kMaxIpcRecursionDepth);
  RETURN_NOT_OK(serializer.GetTotalSize(size));
  return Status::OK();
}

// ----------------------------------------------------------------------
// Record batch read path

struct RecordBatchContext {
  const RecordBatchMetadata* metadata;
  int buffer_index;
  int field_index;
  int max_recursion_depth;
};

// Traverse the flattened record batch metadata and reassemble the
// corresponding array containers
class ArrayLoader : public TypeVisitor {
 public:
  ArrayLoader(
      const Field& field, RecordBatchContext* context, io::ReadableFileInterface* file)
      : field_(field), context_(context), file_(file) {}

  Status Load(std::shared_ptr<Array>* out) {
    if (context_->max_recursion_depth <= 0) {
      return Status::Invalid("Max recursion depth reached");
    }

    // Load the array
    RETURN_NOT_OK(field_.type->Accept(this));

    *out = std::move(result_);
    return Status::OK();
  }

 private:
  const Field& field_;
  RecordBatchContext* context_;
  io::ReadableFileInterface* file_;

  // Used in visitor pattern
  std::shared_ptr<Array> result_;

  Status LoadChild(const Field& field, std::shared_ptr<Array>* out) {
    ArrayLoader loader(field, context_, file_);
    --context_->max_recursion_depth;
    RETURN_NOT_OK(loader.Load(out));
    ++context_->max_recursion_depth;
    return Status::OK();
  }

  Status GetBuffer(int buffer_index, std::shared_ptr<Buffer>* out) {
    BufferMetadata metadata = context_->metadata->buffer(buffer_index);

    if (metadata.length == 0) {
      *out = std::make_shared<Buffer>(nullptr, 0);
      return Status::OK();
    } else {
      return file_->ReadAt(metadata.offset, metadata.length, out);
    }
  }

  Status LoadCommon(FieldMetadata* field_meta, std::shared_ptr<Buffer>* null_bitmap) {
    // pop off a field
    if (context_->field_index >= context_->metadata->num_fields()) {
      return Status::Invalid("Ran out of field metadata, likely malformed");
    }

    // This only contains the length and null count, which we need to figure
    // out what to do with the buffers. For example, if null_count == 0, then
    // we can skip that buffer without reading from shared memory
    *field_meta = context_->metadata->field(context_->field_index++);

    // extract null_bitmap which is common to all arrays
    if (field_meta->null_count == 0) {
      *null_bitmap = nullptr;
    } else {
      RETURN_NOT_OK(GetBuffer(context_->buffer_index, null_bitmap));
    }
    context_->buffer_index++;
    return Status::OK();
  }

  Status LoadPrimitive(const DataType& type) {
    FieldMetadata field_meta;
    std::shared_ptr<Buffer> null_bitmap;
    RETURN_NOT_OK(LoadCommon(&field_meta, &null_bitmap));

    std::shared_ptr<Buffer> data;
    if (field_meta.length > 0) {
      RETURN_NOT_OK(GetBuffer(context_->buffer_index++, &data));
    } else {
      context_->buffer_index++;
      data.reset(new Buffer(nullptr, 0));
    }
    return MakePrimitiveArray(field_.type, field_meta.length, data, field_meta.null_count,
        null_bitmap, &result_);
  }

  template <typename CONTAINER>
  Status LoadBinary() {
    FieldMetadata field_meta;
    std::shared_ptr<Buffer> null_bitmap;
    RETURN_NOT_OK(LoadCommon(&field_meta, &null_bitmap));

    std::shared_ptr<Buffer> offsets;
    std::shared_ptr<Buffer> values;
    RETURN_NOT_OK(GetBuffer(context_->buffer_index++, &offsets));
    RETURN_NOT_OK(GetBuffer(context_->buffer_index++, &values));

    result_ = std::make_shared<CONTAINER>(
        field_meta.length, offsets, values, field_meta.null_count, null_bitmap);
    return Status::OK();
  }

  Status Visit(const NullType& type) override { return Status::NotImplemented("null"); }

  Status Visit(const BooleanType& type) override { return LoadPrimitive(type); }

  Status Visit(const Int8Type& type) override { return LoadPrimitive(type); }

  Status Visit(const Int16Type& type) override { return LoadPrimitive(type); }

  Status Visit(const Int32Type& type) override { return LoadPrimitive(type); }

  Status Visit(const Int64Type& type) override { return LoadPrimitive(type); }

  Status Visit(const UInt8Type& type) override { return LoadPrimitive(type); }

  Status Visit(const UInt16Type& type) override { return LoadPrimitive(type); }

  Status Visit(const UInt32Type& type) override { return LoadPrimitive(type); }

  Status Visit(const UInt64Type& type) override { return LoadPrimitive(type); }

  Status Visit(const HalfFloatType& type) override { return LoadPrimitive(type); }

  Status Visit(const FloatType& type) override { return LoadPrimitive(type); }

  Status Visit(const DoubleType& type) override { return LoadPrimitive(type); }

  Status Visit(const StringType& type) override { return LoadBinary<StringArray>(); }

  Status Visit(const BinaryType& type) override { return LoadBinary<BinaryArray>(); }

  Status Visit(const DateType& type) override { return LoadPrimitive(type); }

  Status Visit(const TimeType& type) override { return LoadPrimitive(type); }

  Status Visit(const TimestampType& type) override { return LoadPrimitive(type); }

  Status Visit(const IntervalType& type) override {
    return Status::NotImplemented(type.ToString());
  }

  Status Visit(const DecimalType& type) override {
    return Status::NotImplemented(type.ToString());
  }

  Status Visit(const ListType& type) override {
    FieldMetadata field_meta;
    std::shared_ptr<Buffer> null_bitmap;
    std::shared_ptr<Buffer> offsets;

    RETURN_NOT_OK(LoadCommon(&field_meta, &null_bitmap));
    RETURN_NOT_OK(GetBuffer(context_->buffer_index++, &offsets));

    const int num_children = type.num_children();
    if (num_children != 1) {
      std::stringstream ss;
      ss << "Wrong number of children: " << num_children;
      return Status::Invalid(ss.str());
    }
    std::shared_ptr<Array> values_array;

    RETURN_NOT_OK(LoadChild(*type.child(0).get(), &values_array));

    result_ = std::make_shared<ListArray>(field_.type, field_meta.length, offsets,
        values_array, field_meta.null_count, null_bitmap);
    return Status::OK();
  }

  Status Visit(const StructType& type) override {
    FieldMetadata field_meta;
    std::shared_ptr<Buffer> null_bitmap;
    RETURN_NOT_OK(LoadCommon(&field_meta, &null_bitmap));

    const int num_children = type.num_children();
    std::vector<ArrayPtr> fields;
    fields.reserve(num_children);

    for (int child_idx = 0; child_idx < num_children; ++child_idx) {
      std::shared_ptr<Array> field_array;
      RETURN_NOT_OK(LoadChild(*type.child(child_idx).get(), &field_array));
      fields.emplace_back(field_array);
    }

    result_ = std::make_shared<StructArray>(
        field_.type, field_meta.length, fields, field_meta.null_count, null_bitmap);
    return Status::OK();
  }

  Status Visit(const UnionType& type) override {
    return Status::NotImplemented(type.ToString());
  }
};

class RecordBatchReader {
 public:
  RecordBatchReader(const std::shared_ptr<RecordBatchMetadata>& metadata,
      const std::shared_ptr<Schema>& schema, int max_recursion_depth,
      io::ReadableFileInterface* file)
      : metadata_(metadata),
        schema_(schema),
        max_recursion_depth_(max_recursion_depth),
        file_(file) {}

  Status Read(std::shared_ptr<RecordBatch>* out) {
    std::vector<std::shared_ptr<Array>> arrays(schema_->num_fields());

    // The field_index and buffer_index are incremented in the ArrayLoader
    // based on how much of the batch is "consumed" (through nested data
    // reconstruction, for example)
    context_.metadata = metadata_.get();
    context_.field_index = 0;
    context_.buffer_index = 0;
    context_.max_recursion_depth = max_recursion_depth_;

    for (int i = 0; i < schema_->num_fields(); ++i) {
      ArrayLoader loader(*schema_->field(i).get(), &context_, file_);
      RETURN_NOT_OK(loader.Load(&arrays[i]));
    }

    *out = std::make_shared<RecordBatch>(schema_, metadata_->length(), arrays);
    return Status::OK();
  }

 private:
  RecordBatchContext context_;
  std::shared_ptr<RecordBatchMetadata> metadata_;
  std::shared_ptr<Schema> schema_;
  int max_recursion_depth_;
  io::ReadableFileInterface* file_;
};

Status ReadRecordBatchMetadata(int64_t offset, int32_t metadata_length,
    io::ReadableFileInterface* file, std::shared_ptr<RecordBatchMetadata>* metadata) {
  std::shared_ptr<Buffer> buffer;
  RETURN_NOT_OK(file->ReadAt(offset, metadata_length, &buffer));

  int32_t flatbuffer_size = *reinterpret_cast<const int32_t*>(buffer->data());

  if (flatbuffer_size + static_cast<int>(sizeof(int32_t)) > metadata_length) {
    std::stringstream ss;
    ss << "flatbuffer size " << metadata_length << " invalid. File offset: " << offset
       << ", metadata length: " << metadata_length;
    return Status::Invalid(ss.str());
  }

  *metadata = std::make_shared<RecordBatchMetadata>(buffer, sizeof(int32_t));
  return Status::OK();
}

Status ReadRecordBatch(const std::shared_ptr<RecordBatchMetadata>& metadata,
    const std::shared_ptr<Schema>& schema, io::ReadableFileInterface* file,
    std::shared_ptr<RecordBatch>* out) {
  return ReadRecordBatch(metadata, schema, kMaxIpcRecursionDepth, file, out);
}

Status ReadRecordBatch(const std::shared_ptr<RecordBatchMetadata>& metadata,
    const std::shared_ptr<Schema>& schema, int max_recursion_depth,
    io::ReadableFileInterface* file, std::shared_ptr<RecordBatch>* out) {
  RecordBatchReader reader(metadata, schema, max_recursion_depth, file);
  return reader.Read(out);
}

}  // namespace ipc
}  // namespace arrow
