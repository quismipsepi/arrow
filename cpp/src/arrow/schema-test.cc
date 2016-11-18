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
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "arrow/schema.h"
#include "arrow/type.h"

using std::shared_ptr;
using std::vector;

namespace arrow {

TEST(TestField, Basics) {
  Field f0("f0", int32());
  Field f0_nn("f0", int32(), false);

  ASSERT_EQ(f0.name, "f0");
  ASSERT_EQ(f0.type->ToString(), int32()->ToString());

  ASSERT_TRUE(f0.nullable);
  ASSERT_FALSE(f0_nn.nullable);
}

TEST(TestField, Equals) {
  Field f0("f0", int32());
  Field f0_nn("f0", int32(), false);
  Field f0_other("f0", int32());

  ASSERT_EQ(f0, f0_other);
  ASSERT_NE(f0, f0_nn);
}

class TestSchema : public ::testing::Test {
 public:
  void SetUp() {}
};

TEST_F(TestSchema, Basics) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f1_optional = field("f1", uint8());

  auto f2 = field("f2", utf8());

  vector<shared_ptr<Field>> fields = {f0, f1, f2};
  auto schema = std::make_shared<Schema>(fields);

  ASSERT_EQ(3, schema->num_fields());
  ASSERT_EQ(f0, schema->field(0));
  ASSERT_EQ(f1, schema->field(1));
  ASSERT_EQ(f2, schema->field(2));

  auto schema2 = std::make_shared<Schema>(fields);

  vector<shared_ptr<Field>> fields3 = {f0, f1_optional, f2};
  auto schema3 = std::make_shared<Schema>(fields3);
  ASSERT_TRUE(schema->Equals(schema2));
  ASSERT_FALSE(schema->Equals(schema3));

  ASSERT_TRUE(schema->Equals(*schema2.get()));
  ASSERT_FALSE(schema->Equals(*schema3.get()));
}

TEST_F(TestSchema, ToString) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", list(int16()));

  vector<shared_ptr<Field>> fields = {f0, f1, f2, f3};
  auto schema = std::make_shared<Schema>(fields);

  std::string result = schema->ToString();
  std::string expected = R"(f0: int32
f1: uint8 not null
f2: string
f3: list<item: int16>)";

  ASSERT_EQ(expected, result);
}

TEST_F(TestSchema, GetFieldByName) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", list(int16()));

  vector<shared_ptr<Field>> fields = {f0, f1, f2, f3};
  auto schema = std::make_shared<Schema>(fields);

  std::shared_ptr<Field> result;

  result = schema->GetFieldByName("f1");
  ASSERT_TRUE(f1->Equals(result));

  result = schema->GetFieldByName("f3");
  ASSERT_TRUE(f3->Equals(result));

  result = schema->GetFieldByName("not-found");
  ASSERT_TRUE(result == nullptr);
}

}  // namespace arrow
