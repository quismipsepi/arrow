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

#include "arrow/buffer.h"

#include <cstdint>
#include <limits>

#include "arrow/memory_pool.h"
#include "arrow/status.h"
#include "arrow/util/bit-util.h"
#include "arrow/util/logging.h"

namespace arrow {

Buffer::Buffer(const std::shared_ptr<Buffer>& parent, int64_t offset, int64_t size) {
  data_ = parent->data() + offset;
  size_ = size;
  parent_ = parent;
  capacity_ = size;
}

Buffer::~Buffer() {}

Status Buffer::Copy(
    int64_t start, int64_t nbytes, MemoryPool* pool, std::shared_ptr<Buffer>* out) const {
  // Sanity checks
  DCHECK_LT(start, size_);
  DCHECK_LE(nbytes, size_ - start);

  auto new_buffer = std::make_shared<PoolBuffer>(pool);
  RETURN_NOT_OK(new_buffer->Resize(nbytes));

  std::memcpy(new_buffer->mutable_data(), data() + start, nbytes);

  *out = new_buffer;
  return Status::OK();
}

Status Buffer::Copy(int64_t start, int64_t nbytes, std::shared_ptr<Buffer>* out) const {
  return Copy(start, nbytes, default_memory_pool(), out);
}

std::shared_ptr<Buffer> SliceBuffer(
    const std::shared_ptr<Buffer>& buffer, int64_t offset, int64_t length) {
  DCHECK_LT(offset, buffer->size());
  DCHECK_LE(length, buffer->size() - offset);
  return std::make_shared<Buffer>(buffer, offset, length);
}

std::shared_ptr<Buffer> MutableBuffer::GetImmutableView() {
  return std::make_shared<Buffer>(this->get_shared_ptr(), 0, size());
}

PoolBuffer::PoolBuffer(MemoryPool* pool) : ResizableBuffer(nullptr, 0) {
  if (pool == nullptr) { pool = default_memory_pool(); }
  pool_ = pool;
}

PoolBuffer::~PoolBuffer() {
  if (mutable_data_ != nullptr) { pool_->Free(mutable_data_, capacity_); }
}

Status PoolBuffer::Reserve(int64_t new_capacity) {
  if (!mutable_data_ || new_capacity > capacity_) {
    uint8_t* new_data;
    new_capacity = BitUtil::RoundUpToMultipleOf64(new_capacity);
    if (mutable_data_) {
      RETURN_NOT_OK(pool_->Reallocate(capacity_, new_capacity, &mutable_data_));
    } else {
      RETURN_NOT_OK(pool_->Allocate(new_capacity, &new_data));
      mutable_data_ = new_data;
    }
    data_ = mutable_data_;
    capacity_ = new_capacity;
  }
  return Status::OK();
}

Status PoolBuffer::Resize(int64_t new_size) {
  if (new_size > size_) {
    RETURN_NOT_OK(Reserve(new_size));
  } else {
    // Buffer is not growing, so shrink to the requested size without
    // excess space.
    if (capacity_ != new_size) {
      // Buffer hasn't got yet the requested size.
      if (new_size == 0) {
        pool_->Free(mutable_data_, capacity_);
        capacity_ = 0;
        mutable_data_ = nullptr;
        data_ = nullptr;
      } else {
        RETURN_NOT_OK(pool_->Reallocate(capacity_, new_size, &mutable_data_));
        data_ = mutable_data_;
        capacity_ = new_size;
      }
    }
  }
  size_ = new_size;
  return Status::OK();
}

}  // namespace arrow
