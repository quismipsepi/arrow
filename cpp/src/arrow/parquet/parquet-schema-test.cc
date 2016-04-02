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

#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "arrow/test-util.h"
#include "arrow/type.h"
#include "arrow/types/decimal.h"
#include "arrow/util/status.h"

#include "arrow/parquet/schema.h"

using ParquetType = parquet::Type;
using parquet::LogicalType;
using parquet::Repetition;
using parquet::schema::NodePtr;
using parquet::schema::GroupNode;
using parquet::schema::PrimitiveNode;

namespace arrow {

namespace parquet {

const auto BOOL = std::make_shared<BooleanType>();
const auto UINT8 = std::make_shared<UInt8Type>();
const auto INT32 = std::make_shared<Int32Type>();
const auto INT64 = std::make_shared<Int64Type>();
const auto FLOAT = std::make_shared<FloatType>();
const auto DOUBLE = std::make_shared<DoubleType>();
const auto UTF8 = std::make_shared<StringType>();
const auto BINARY = std::make_shared<ListType>(std::make_shared<Field>("", UINT8));
const auto DECIMAL_8_4 = std::make_shared<DecimalType>(8, 4);

class TestConvertParquetSchema : public ::testing::Test {
 public:
  virtual void SetUp() {}

  void CheckFlatSchema(const std::shared_ptr<Schema>& expected_schema) {
    ASSERT_EQ(expected_schema->num_fields(), result_schema_->num_fields());
    for (int i = 0; i < expected_schema->num_fields(); ++i) {
      auto lhs = result_schema_->field(i);
      auto rhs = expected_schema->field(i);
      EXPECT_TRUE(lhs->Equals(rhs)) << i << " " << lhs->ToString()
                                    << " != " << rhs->ToString();
    }
  }

  Status ConvertSchema(const std::vector<NodePtr>& nodes) {
    NodePtr schema = GroupNode::Make("schema", Repetition::REPEATED, nodes);
    descr_.Init(schema);
    return FromParquetSchema(&descr_, &result_schema_);
  }

 protected:
  ::parquet::SchemaDescriptor descr_;
  std::shared_ptr<Schema> result_schema_;
};

TEST_F(TestConvertParquetSchema, ParquetFlatPrimitives) {
  std::vector<NodePtr> parquet_fields;
  std::vector<std::shared_ptr<Field>> arrow_fields;

  parquet_fields.push_back(
      PrimitiveNode::Make("boolean", Repetition::REQUIRED, ParquetType::BOOLEAN));
  arrow_fields.push_back(std::make_shared<Field>("boolean", BOOL, false));

  parquet_fields.push_back(
      PrimitiveNode::Make("int32", Repetition::REQUIRED, ParquetType::INT32));
  arrow_fields.push_back(std::make_shared<Field>("int32", INT32, false));

  parquet_fields.push_back(
      PrimitiveNode::Make("int64", Repetition::REQUIRED, ParquetType::INT64));
  arrow_fields.push_back(std::make_shared<Field>("int64", INT64, false));

  parquet_fields.push_back(
      PrimitiveNode::Make("float", Repetition::OPTIONAL, ParquetType::FLOAT));
  arrow_fields.push_back(std::make_shared<Field>("float", FLOAT));

  parquet_fields.push_back(
      PrimitiveNode::Make("double", Repetition::OPTIONAL, ParquetType::DOUBLE));
  arrow_fields.push_back(std::make_shared<Field>("double", DOUBLE));

  parquet_fields.push_back(
      PrimitiveNode::Make("binary", Repetition::OPTIONAL, ParquetType::BYTE_ARRAY));
  arrow_fields.push_back(std::make_shared<Field>("binary", BINARY));

  parquet_fields.push_back(PrimitiveNode::Make(
      "string", Repetition::OPTIONAL, ParquetType::BYTE_ARRAY, LogicalType::UTF8));
  arrow_fields.push_back(std::make_shared<Field>("string", UTF8));

  parquet_fields.push_back(PrimitiveNode::Make("flba-binary", Repetition::OPTIONAL,
      ParquetType::FIXED_LEN_BYTE_ARRAY, LogicalType::NONE, 12));
  arrow_fields.push_back(std::make_shared<Field>("flba-binary", BINARY));

  auto arrow_schema = std::make_shared<Schema>(arrow_fields);
  ASSERT_OK(ConvertSchema(parquet_fields));

  CheckFlatSchema(arrow_schema);
}

TEST_F(TestConvertParquetSchema, ParquetFlatDecimals) {
  std::vector<NodePtr> parquet_fields;
  std::vector<std::shared_ptr<Field>> arrow_fields;

  parquet_fields.push_back(PrimitiveNode::Make("flba-decimal", Repetition::OPTIONAL,
      ParquetType::FIXED_LEN_BYTE_ARRAY, LogicalType::DECIMAL, 4, 8, 4));
  arrow_fields.push_back(std::make_shared<Field>("flba-decimal", DECIMAL_8_4));

  parquet_fields.push_back(PrimitiveNode::Make("binary-decimal", Repetition::OPTIONAL,
      ParquetType::BYTE_ARRAY, LogicalType::DECIMAL, -1, 8, 4));
  arrow_fields.push_back(std::make_shared<Field>("binary-decimal", DECIMAL_8_4));

  parquet_fields.push_back(PrimitiveNode::Make("int32-decimal", Repetition::OPTIONAL,
      ParquetType::INT32, LogicalType::DECIMAL, -1, 8, 4));
  arrow_fields.push_back(std::make_shared<Field>("int32-decimal", DECIMAL_8_4));

  parquet_fields.push_back(PrimitiveNode::Make("int64-decimal", Repetition::OPTIONAL,
      ParquetType::INT64, LogicalType::DECIMAL, -1, 8, 4));
  arrow_fields.push_back(std::make_shared<Field>("int64-decimal", DECIMAL_8_4));

  auto arrow_schema = std::make_shared<Schema>(arrow_fields);
  ASSERT_OK(ConvertSchema(parquet_fields));

  CheckFlatSchema(arrow_schema);
}

TEST_F(TestConvertParquetSchema, UnsupportedThings) {
  std::vector<NodePtr> unsupported_nodes;

  unsupported_nodes.push_back(
      PrimitiveNode::Make("int96", Repetition::REQUIRED, ParquetType::INT96));

  unsupported_nodes.push_back(
      GroupNode::Make("repeated-group", Repetition::REPEATED, {}));

  unsupported_nodes.push_back(PrimitiveNode::Make(
      "int32", Repetition::OPTIONAL, ParquetType::INT32, LogicalType::DATE));

  unsupported_nodes.push_back(PrimitiveNode::Make(
      "int64", Repetition::OPTIONAL, ParquetType::INT64, LogicalType::TIMESTAMP_MILLIS));

  for (const NodePtr& node : unsupported_nodes) {
    ASSERT_RAISES(NotImplemented, ConvertSchema({node}));
  }
}

TEST(TestNodeConversion, DateAndTime) {}

}  // namespace parquet

}  // namespace arrow
