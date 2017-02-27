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

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <vector>

#include "gtest/gtest.h"

#include "arrow/array.h"
#include "arrow/buffer.h"
#include "arrow/memory_pool.h"
#include "arrow/test-util.h"
#include "arrow/type.h"

namespace arrow {

class TestArray : public ::testing::Test {
 public:
  void SetUp() { pool_ = default_memory_pool(); }

 protected:
  MemoryPool* pool_;
};

TEST_F(TestArray, TestNullCount) {
  auto data = std::make_shared<PoolBuffer>(pool_);
  auto null_bitmap = std::make_shared<PoolBuffer>(pool_);

  std::unique_ptr<Int32Array> arr(new Int32Array(100, data, null_bitmap, 10));
  ASSERT_EQ(10, arr->null_count());

  std::unique_ptr<Int32Array> arr_no_nulls(new Int32Array(100, data));
  ASSERT_EQ(0, arr_no_nulls->null_count());
}

TEST_F(TestArray, TestLength) {
  auto data = std::make_shared<PoolBuffer>(pool_);
  std::unique_ptr<Int32Array> arr(new Int32Array(100, data));
  ASSERT_EQ(arr->length(), 100);
}

std::shared_ptr<Array> MakeArrayFromValidBytes(
    const std::vector<uint8_t>& v, MemoryPool* pool) {
  int64_t null_count = v.size() - std::accumulate(v.begin(), v.end(), 0);
  std::shared_ptr<Buffer> null_buf = test::bytes_to_null_buffer(v);

  BufferBuilder value_builder(pool);
  for (size_t i = 0; i < v.size(); ++i) {
    value_builder.Append<int32_t>(0);
  }

  std::shared_ptr<Array> arr(
      new Int32Array(v.size(), value_builder.Finish(), null_buf, null_count));
  return arr;
}

TEST_F(TestArray, TestEquality) {
  auto array = MakeArrayFromValidBytes({1, 0, 1, 1, 0, 1, 0, 0}, pool_);
  auto equal_array = MakeArrayFromValidBytes({1, 0, 1, 1, 0, 1, 0, 0}, pool_);
  auto unequal_array = MakeArrayFromValidBytes({1, 1, 1, 1, 0, 1, 0, 0}, pool_);

  EXPECT_TRUE(array->Equals(array));
  EXPECT_TRUE(array->Equals(equal_array));
  EXPECT_TRUE(equal_array->Equals(array));
  EXPECT_FALSE(equal_array->Equals(unequal_array));
  EXPECT_FALSE(unequal_array->Equals(equal_array));
  EXPECT_TRUE(array->RangeEquals(4, 8, 4, unequal_array));
  EXPECT_FALSE(array->RangeEquals(0, 4, 0, unequal_array));
  EXPECT_FALSE(array->RangeEquals(0, 8, 0, unequal_array));
  EXPECT_FALSE(array->RangeEquals(1, 2, 1, unequal_array));
}

TEST_F(TestArray, SliceRecomputeNullCount) {
  std::vector<uint8_t> valid_bytes = {1, 0, 1, 1, 0, 1, 0, 0, 0};

  auto array = MakeArrayFromValidBytes(valid_bytes, pool_);

  ASSERT_EQ(5, array->null_count());

  auto slice = array->Slice(1, 4);
  ASSERT_EQ(2, slice->null_count());

  slice = array->Slice(4);
  ASSERT_EQ(4, slice->null_count());

  slice = array->Slice(0);
  ASSERT_EQ(5, slice->null_count());

  // No bitmap, compute 0
  std::shared_ptr<MutableBuffer> data;
  const int kBufferSize = 64;
  ASSERT_OK(AllocateBuffer(pool_, kBufferSize, &data));
  memset(data->mutable_data(), 0, kBufferSize);

  auto arr = std::make_shared<Int32Array>(16, data, nullptr, -1);
  ASSERT_EQ(0, arr->null_count());
}

TEST_F(TestArray, TestIsNull) {
  // clang-format off
  std::vector<uint8_t> null_bitmap = {1, 0, 1, 1, 0, 1, 0, 0,
                                      1, 0, 1, 1, 0, 1, 0, 0,
                                      1, 0, 1, 1, 0, 1, 0, 0,
                                      1, 0, 1, 1, 0, 1, 0, 0,
                                      1, 0, 0, 1};
  // clang-format on
  int64_t null_count = 0;
  for (uint8_t x : null_bitmap) {
    if (x == 0) { ++null_count; }
  }

  std::shared_ptr<Buffer> null_buf = test::bytes_to_null_buffer(null_bitmap);
  std::unique_ptr<Array> arr;
  arr.reset(new Int32Array(null_bitmap.size(), nullptr, null_buf, null_count));

  ASSERT_EQ(null_count, arr->null_count());
  ASSERT_EQ(5, null_buf->size());

  ASSERT_TRUE(arr->null_bitmap()->Equals(*null_buf.get()));

  for (size_t i = 0; i < null_bitmap.size(); ++i) {
    EXPECT_EQ(null_bitmap[i], !arr->IsNull(i)) << i;
  }
}

TEST_F(TestArray, BuildLargeInMemoryArray) {
  const int64_t length = static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1;

  BooleanBuilder builder(default_memory_pool());
  ASSERT_OK(builder.Reserve(length));
  ASSERT_OK(builder.Advance(length));

  std::shared_ptr<Array> result;
  ASSERT_OK(builder.Finish(&result));

  ASSERT_EQ(length, result->length());
}

TEST_F(TestArray, TestCopy) {}

}  // namespace arrow
