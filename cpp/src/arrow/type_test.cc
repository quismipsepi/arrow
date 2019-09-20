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

// Unit tests for DataType (and subclasses), Field, and Schema

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "arrow/memory_pool.h"
#include "arrow/testing/gtest_util.h"
#include "arrow/testing/util.h"
#include "arrow/type.h"
#include "arrow/type_traits.h"
#include "arrow/util/checked_cast.h"
#include "arrow/util/key_value_metadata.h"

namespace arrow {

using internal::checked_cast;

template <typename T>
void AssertFingerprintablesEqual(const T& left, const T& right, bool check_metadata,
                                 const char* types_plural) {
  ASSERT_TRUE(left.Equals(right, check_metadata))
      << types_plural << " '" << left.ToString() << "' and '" << right.ToString()
      << "' should have compared equal";
  auto lfp = left.fingerprint();
  auto rfp = right.fingerprint();
  // All types tested in this file should implement fingerprinting
  ASSERT_NE(lfp, "") << "fingerprint for '" << left.ToString() << "' should not be empty";
  ASSERT_NE(rfp, "") << "fingerprint for '" << right.ToString()
                     << "' should not be empty";
  if (check_metadata) {
    lfp += left.metadata_fingerprint();
    rfp += right.metadata_fingerprint();
  }
  ASSERT_EQ(lfp, rfp) << "Fingerprints for " << types_plural << " '" << left.ToString()
                      << "' and '" << right.ToString() << "' should have compared equal";
}

template <typename T>
void AssertFingerprintablesNotEqual(const T& left, const T& right, bool check_metadata,
                                    const char* types_plural) {
  ASSERT_FALSE(left.Equals(right, check_metadata))
      << types_plural << " '" << left.ToString() << "' and '" << right.ToString()
      << "' should have compared unequal";
  auto lfp = left.fingerprint();
  auto rfp = right.fingerprint();
  // All types tested in this file should implement fingerprinting
  ASSERT_NE(lfp, "") << "fingerprint for '" << left.ToString() << "' should not be empty";
  ASSERT_NE(rfp, "") << "fingerprint for '" << right.ToString()
                     << "' should not be empty";
  if (check_metadata) {
    lfp += left.metadata_fingerprint();
    rfp += right.metadata_fingerprint();
  }
  ASSERT_NE(lfp, rfp) << "Fingerprints for " << types_plural << " '" << left.ToString()
                      << "' and '" << right.ToString()
                      << "' should have compared unequal";
}

void AssertTypesEqual(const DataType& left, const DataType& right,
                      bool check_metadata = true) {
  AssertFingerprintablesEqual(left, right, check_metadata, "types");
}

void AssertTypesNotEqual(const DataType& left, const DataType& right,
                         bool check_metadata = true) {
  AssertFingerprintablesNotEqual(left, right, check_metadata, "types");
}

void AssertFieldsEqual(const Field& left, const Field& right,
                       bool check_metadata = true) {
  AssertFingerprintablesEqual(left, right, check_metadata, "fields");
}

void AssertFieldsNotEqual(const Field& left, const Field& right,
                          bool check_metadata = true) {
  AssertFingerprintablesNotEqual(left, right, check_metadata, "fields");
}

TEST(TestField, Basics) {
  Field f0("f0", int32());
  Field f0_nn("f0", int32(), false);

  ASSERT_EQ(f0.name(), "f0");
  ASSERT_EQ(f0.type()->ToString(), int32()->ToString());

  ASSERT_TRUE(f0.nullable());
  ASSERT_FALSE(f0_nn.nullable());
}

TEST(TestField, Equals) {
  auto meta1 = key_value_metadata({{"a", "1"}, {"b", "2"}});
  // Different from meta1
  auto meta2 = key_value_metadata({{"a", "1"}, {"b", "3"}});
  // Equal to meta1, though in different order
  auto meta3 = key_value_metadata({{"b", "2"}, {"a", "1"}});

  Field f0("f0", int32());
  Field f0_nn("f0", int32(), false);
  Field f0_other("f0", int32());
  Field f0_with_meta1("f0", int32(), true, meta1);
  Field f0_with_meta2("f0", int32(), true, meta2);
  Field f0_with_meta3("f0", int32(), true, meta3);

  AssertFieldsEqual(f0, f0_other);
  AssertFieldsNotEqual(f0, f0_nn);
  AssertFieldsNotEqual(f0, f0_with_meta1);
  AssertFieldsNotEqual(f0_with_meta1, f0_with_meta2);
  AssertFieldsEqual(f0_with_meta1, f0_with_meta3);

  AssertFieldsEqual(f0, f0_with_meta1, false);
  AssertFieldsEqual(f0, f0_with_meta2, false);
  AssertFieldsEqual(f0_with_meta1, f0_with_meta2, false);
}

TEST(TestField, TestMetadataConstruction) {
  auto metadata = std::shared_ptr<KeyValueMetadata>(
      new KeyValueMetadata({"foo", "bar"}, {"bizz", "buzz"}));
  auto metadata2 = metadata->Copy();
  auto f0 = field("f0", int32(), true, metadata);
  auto f1 = field("f0", int32(), true, metadata2);
  ASSERT_TRUE(metadata->Equals(*f0->metadata()));
  AssertFieldsEqual(*f0, *f1);
}

TEST(TestField, TestWithMetadata) {
  auto metadata = std::shared_ptr<KeyValueMetadata>(
      new KeyValueMetadata({"foo", "bar"}, {"bizz", "buzz"}));
  auto f0 = field("f0", int32());
  auto f1 = field("f0", int32(), true, metadata);
  std::shared_ptr<Field> f2 = f0->WithMetadata(metadata);

  AssertFieldsEqual(*f1, *f2);
  AssertFieldsNotEqual(*f0, *f2);
  ASSERT_TRUE(f1->Equals(f2, /*check_metadata =*/false));
  ASSERT_TRUE(f0->Equals(f2, /*check_metadata =*/false));

  // Not copied
  ASSERT_TRUE(metadata.get() == f1->metadata().get());
}

TEST(TestField, TestRemoveMetadata) {
  auto metadata = std::shared_ptr<KeyValueMetadata>(
      new KeyValueMetadata({"foo", "bar"}, {"bizz", "buzz"}));
  auto f0 = field("f0", int32());
  auto f1 = field("f0", int32(), true, metadata);
  std::shared_ptr<Field> f2 = f1->RemoveMetadata();
  ASSERT_TRUE(f2->metadata() == nullptr);
}

TEST(TestField, TestEmptyMetadata) {
  // Empty metadata should be equivalent to no metadata at all
  auto metadata1 = key_value_metadata({});
  auto metadata2 = key_value_metadata({"foo"}, {"foo value"});

  auto f0 = field("f0", int32());
  auto f1 = field("f0", int32(), true, metadata1);
  auto f2 = field("f0", int32(), true, metadata2);

  AssertFieldsEqual(*f0, *f1);
  AssertFieldsNotEqual(*f0, *f2);
  ASSERT_TRUE(f0->Equals(f1, /*check_metadata =*/false));
  ASSERT_TRUE(f0->Equals(f2, /*check_metadata =*/false));
}

TEST(TestField, TestFlatten) {
  auto metadata = std::shared_ptr<KeyValueMetadata>(
      new KeyValueMetadata({"foo", "bar"}, {"bizz", "buzz"}));
  auto f0 = field("f0", int32(), true /* nullable */, metadata);
  auto vec = f0->Flatten();
  ASSERT_EQ(vec.size(), 1);
  ASSERT_TRUE(vec[0]->Equals(*f0));

  auto f1 = field("f1", float64(), false /* nullable */);
  auto ff = field("nest", struct_({f0, f1}));
  vec = ff->Flatten();
  ASSERT_EQ(vec.size(), 2);
  auto expected0 = field("nest.f0", int32(), true /* nullable */, metadata);
  // nullable parent implies nullable flattened child
  auto expected1 = field("nest.f1", float64(), true /* nullable */);
  ASSERT_TRUE(vec[0]->Equals(*expected0));
  ASSERT_TRUE(vec[1]->Equals(*expected1));

  ff = field("nest", struct_({f0, f1}), false /* nullable */);
  vec = ff->Flatten();
  ASSERT_EQ(vec.size(), 2);
  expected0 = field("nest.f0", int32(), true /* nullable */, metadata);
  expected1 = field("nest.f1", float64(), false /* nullable */);
  ASSERT_TRUE(vec[0]->Equals(*expected0));
  ASSERT_TRUE(vec[1]->Equals(*expected1));
}

TEST(TestField, TestReplacement) {
  auto metadata = std::shared_ptr<KeyValueMetadata>(
      new KeyValueMetadata({"foo", "bar"}, {"bizz", "buzz"}));
  auto f0 = field("f0", int32(), true, metadata);
  auto fzero = f0->WithType(utf8());
  auto f1 = f0->WithName("f1");

  AssertFieldsNotEqual(*f0, *fzero);
  AssertFieldsNotEqual(*fzero, *f1);
  AssertFieldsNotEqual(*f1, *f0);

  ASSERT_EQ(fzero->name(), "f0");
  ASSERT_TRUE(fzero->type()->Equals(utf8()));
  ASSERT_TRUE(fzero->metadata()->Equals(*metadata));

  ASSERT_EQ(f1->name(), "f1");
  ASSERT_TRUE(f1->type()->Equals(int32()));
  ASSERT_TRUE(f1->metadata()->Equals(*metadata));
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

  auto schema = ::arrow::schema({f0, f1, f2});

  ASSERT_EQ(3, schema->num_fields());
  ASSERT_TRUE(f0->Equals(schema->field(0)));
  ASSERT_TRUE(f1->Equals(schema->field(1)));
  ASSERT_TRUE(f2->Equals(schema->field(2)));

  auto schema2 = ::arrow::schema({f0, f1, f2});

  std::vector<std::shared_ptr<Field>> fields3 = {f0, f1_optional, f2};
  auto schema3 = std::make_shared<Schema>(fields3);
  ASSERT_TRUE(schema->Equals(*schema2));
  ASSERT_FALSE(schema->Equals(*schema3));
  ASSERT_EQ(schema->fingerprint(), schema2->fingerprint());
  ASSERT_NE(schema->fingerprint(), schema3->fingerprint());
}

TEST_F(TestSchema, ToString) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", list(int16()));

  auto schema = ::arrow::schema({f0, f1, f2, f3});

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

  auto schema = ::arrow::schema({f0, f1, f2, f3});

  std::shared_ptr<Field> result;

  result = schema->GetFieldByName("f1");
  ASSERT_TRUE(f1->Equals(result));

  result = schema->GetFieldByName("f3");
  ASSERT_TRUE(f3->Equals(result));

  result = schema->GetFieldByName("not-found");
  ASSERT_TRUE(result == nullptr);
}

TEST_F(TestSchema, GetFieldIndex) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", list(int16()));

  auto schema = ::arrow::schema({f0, f1, f2, f3});

  ASSERT_EQ(0, schema->GetFieldIndex(f0->name()));
  ASSERT_EQ(1, schema->GetFieldIndex(f1->name()));
  ASSERT_EQ(2, schema->GetFieldIndex(f2->name()));
  ASSERT_EQ(3, schema->GetFieldIndex(f3->name()));
  ASSERT_EQ(-1, schema->GetFieldIndex("not-found"));
}

TEST_F(TestSchema, GetFieldDuplicates) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f1", list(int16()));

  auto schema = ::arrow::schema({f0, f1, f2, f3});

  ASSERT_EQ(0, schema->GetFieldIndex(f0->name()));
  ASSERT_EQ(-1, schema->GetFieldIndex(f1->name()));  // duplicate
  ASSERT_EQ(2, schema->GetFieldIndex(f2->name()));
  ASSERT_EQ(-1, schema->GetFieldIndex("not-found"));
  ASSERT_EQ(std::vector<int>{0}, schema->GetAllFieldIndices(f0->name()));
  AssertSortedEquals(std::vector<int>{1, 3}, schema->GetAllFieldIndices(f1->name()));

  std::vector<std::shared_ptr<Field>> results;

  results = schema->GetAllFieldsByName(f0->name());
  ASSERT_EQ(results.size(), 1);
  ASSERT_TRUE(results[0]->Equals(f0));

  results = schema->GetAllFieldsByName(f1->name());
  ASSERT_EQ(results.size(), 2);
  if (results[0]->type()->id() == Type::UINT8) {
    ASSERT_TRUE(results[0]->Equals(f1));
    ASSERT_TRUE(results[1]->Equals(f3));
  } else {
    ASSERT_TRUE(results[0]->Equals(f3));
    ASSERT_TRUE(results[1]->Equals(f1));
  }

  results = schema->GetAllFieldsByName("not-found");
  ASSERT_EQ(results.size(), 0);
}

TEST_F(TestSchema, TestMetadataConstruction) {
  auto metadata0 = key_value_metadata({{"foo", "bar"}, {"bizz", "buzz"}});
  auto metadata1 = key_value_metadata({{"foo", "baz"}});

  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8(), true);
  auto f3 = field("f2", utf8(), true, metadata1->Copy());

  auto schema0 = ::arrow::schema({f0, f1, f2}, metadata0);
  auto schema1 = ::arrow::schema({f0, f1, f2}, metadata1);
  auto schema2 = ::arrow::schema({f0, f1, f2}, metadata0->Copy());
  auto schema3 = ::arrow::schema({f0, f1, f3}, metadata0->Copy());

  ASSERT_TRUE(metadata0->Equals(*schema0->metadata()));
  ASSERT_TRUE(metadata1->Equals(*schema1->metadata()));
  ASSERT_TRUE(metadata0->Equals(*schema2->metadata()));
  ASSERT_TRUE(schema0->Equals(*schema2));
  ASSERT_FALSE(schema0->Equals(*schema1));
  ASSERT_FALSE(schema2->Equals(*schema1));
  ASSERT_FALSE(schema2->Equals(*schema3));  // Field has different metadata

  ASSERT_EQ(schema0->fingerprint(), schema1->fingerprint());
  ASSERT_EQ(schema0->fingerprint(), schema2->fingerprint());
  ASSERT_EQ(schema0->fingerprint(), schema3->fingerprint());
  ASSERT_NE(schema0->metadata_fingerprint(), schema1->metadata_fingerprint());
  ASSERT_EQ(schema0->metadata_fingerprint(), schema2->metadata_fingerprint());
  ASSERT_NE(schema0->metadata_fingerprint(), schema3->metadata_fingerprint());

  // don't check metadata
  ASSERT_TRUE(schema0->Equals(*schema1, false));
  ASSERT_TRUE(schema2->Equals(*schema1, false));
  ASSERT_TRUE(schema2->Equals(*schema3, false));
}

TEST_F(TestSchema, TestEmptyMetadata) {
  // Empty metadata should be equivalent to no metadata at all
  auto f1 = field("f1", int32());
  auto metadata1 = key_value_metadata({});
  auto metadata2 = key_value_metadata({"foo"}, {"foo value"});

  auto schema1 = ::arrow::schema({f1});
  auto schema2 = ::arrow::schema({f1}, metadata1);
  auto schema3 = ::arrow::schema({f1}, metadata2);

  ASSERT_TRUE(schema1->Equals(*schema2));
  ASSERT_FALSE(schema1->Equals(*schema3));

  ASSERT_EQ(schema1->fingerprint(), schema2->fingerprint());
  ASSERT_EQ(schema1->fingerprint(), schema3->fingerprint());
  ASSERT_EQ(schema1->metadata_fingerprint(), schema2->metadata_fingerprint());
  ASSERT_NE(schema1->metadata_fingerprint(), schema3->metadata_fingerprint());
}

TEST_F(TestSchema, TestWithMetadata) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  std::vector<std::shared_ptr<Field>> fields = {f0, f1, f2};
  auto metadata = std::shared_ptr<KeyValueMetadata>(
      new KeyValueMetadata({"foo", "bar"}, {"bizz", "buzz"}));
  auto schema = std::make_shared<Schema>(fields);
  std::shared_ptr<Schema> new_schema = schema->WithMetadata(metadata);
  ASSERT_TRUE(metadata->Equals(*new_schema->metadata()));

  // Not copied
  ASSERT_TRUE(metadata.get() == new_schema->metadata().get());
}

TEST_F(TestSchema, TestRemoveMetadata) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  std::vector<std::shared_ptr<Field>> fields = {f0, f1, f2};
  KeyValueMetadata metadata({"foo", "bar"}, {"bizz", "buzz"});
  auto schema = std::make_shared<Schema>(fields);
  std::shared_ptr<Schema> new_schema = schema->RemoveMetadata();
  ASSERT_TRUE(new_schema->metadata() == nullptr);
}

#define PRIMITIVE_TEST(KLASS, CTYPE, ENUM, NAME)                              \
  TEST(TypesTest, ARROW_CONCAT(TestPrimitive_, ENUM)) {                       \
    KLASS tp;                                                                 \
                                                                              \
    ASSERT_EQ(tp.id(), Type::ENUM);                                           \
    ASSERT_EQ(tp.ToString(), std::string(NAME));                              \
                                                                              \
    using CType = TypeTraits<KLASS>::CType;                                   \
    static_assert(std::is_same<CType, CTYPE>::value, "Not the same c-type!"); \
                                                                              \
    using DerivedArrowType = CTypeTraits<CTYPE>::ArrowType;                   \
    static_assert(std::is_same<DerivedArrowType, KLASS>::value,               \
                  "Not the same arrow-type!");                                \
  }

PRIMITIVE_TEST(Int8Type, int8_t, INT8, "int8");
PRIMITIVE_TEST(Int16Type, int16_t, INT16, "int16");
PRIMITIVE_TEST(Int32Type, int32_t, INT32, "int32");
PRIMITIVE_TEST(Int64Type, int64_t, INT64, "int64");
PRIMITIVE_TEST(UInt8Type, uint8_t, UINT8, "uint8");
PRIMITIVE_TEST(UInt16Type, uint16_t, UINT16, "uint16");
PRIMITIVE_TEST(UInt32Type, uint32_t, UINT32, "uint32");
PRIMITIVE_TEST(UInt64Type, uint64_t, UINT64, "uint64");

PRIMITIVE_TEST(FloatType, float, FLOAT, "float");
PRIMITIVE_TEST(DoubleType, double, DOUBLE, "double");

PRIMITIVE_TEST(BooleanType, bool, BOOL, "bool");

TEST(TestBinaryType, ToString) {
  BinaryType t1;
  BinaryType e1;
  StringType t2;
  AssertTypesEqual(t1, e1);
  AssertTypesNotEqual(t1, t2);
  ASSERT_EQ(t1.id(), Type::BINARY);
  ASSERT_EQ(t1.ToString(), std::string("binary"));
}

TEST(TestStringType, ToString) {
  StringType str;
  ASSERT_EQ(str.id(), Type::STRING);
  ASSERT_EQ(str.ToString(), std::string("string"));
}

TEST(TestLargeBinaryTypes, ToString) {
  BinaryType bt1;
  LargeBinaryType t1;
  LargeBinaryType e1;
  LargeStringType t2;
  AssertTypesEqual(t1, e1);
  AssertTypesNotEqual(t1, t2);
  AssertTypesNotEqual(t1, bt1);
  ASSERT_EQ(t1.id(), Type::LARGE_BINARY);
  ASSERT_EQ(t1.ToString(), std::string("large_binary"));
  ASSERT_EQ(t2.id(), Type::LARGE_STRING);
  ASSERT_EQ(t2.ToString(), std::string("large_string"));
}

TEST(TestFixedSizeBinaryType, ToString) {
  auto t = fixed_size_binary(10);
  ASSERT_EQ(t->id(), Type::FIXED_SIZE_BINARY);
  ASSERT_EQ("fixed_size_binary[10]", t->ToString());
}

TEST(TestFixedSizeBinaryType, Equals) {
  auto t1 = fixed_size_binary(10);
  auto t2 = fixed_size_binary(10);
  auto t3 = fixed_size_binary(3);

  AssertTypesEqual(*t1, *t2);
  AssertTypesNotEqual(*t1, *t3);
}

TEST(TestListType, Basics) {
  std::shared_ptr<DataType> vt = std::make_shared<UInt8Type>();

  ListType list_type(vt);
  ASSERT_EQ(list_type.id(), Type::LIST);

  ASSERT_EQ("list", list_type.name());
  ASSERT_EQ("list<item: uint8>", list_type.ToString());

  ASSERT_EQ(list_type.value_type()->id(), vt->id());
  ASSERT_EQ(list_type.value_type()->id(), vt->id());

  std::shared_ptr<DataType> st = std::make_shared<StringType>();
  std::shared_ptr<DataType> lt = std::make_shared<ListType>(st);
  ASSERT_EQ("list<item: string>", lt->ToString());

  ListType lt2(lt);
  ASSERT_EQ("list<item: list<item: string>>", lt2.ToString());
}

TEST(TestLargeListType, Basics) {
  std::shared_ptr<DataType> vt = std::make_shared<UInt8Type>();

  LargeListType list_type(vt);
  ASSERT_EQ(list_type.id(), Type::LARGE_LIST);

  ASSERT_EQ("large_list", list_type.name());
  ASSERT_EQ("large_list<item: uint8>", list_type.ToString());

  ASSERT_EQ(list_type.value_type()->id(), vt->id());
  ASSERT_EQ(list_type.value_type()->id(), vt->id());

  std::shared_ptr<DataType> st = std::make_shared<StringType>();
  std::shared_ptr<DataType> lt = std::make_shared<LargeListType>(st);
  ASSERT_EQ("large_list<item: string>", lt->ToString());

  LargeListType lt2(lt);
  ASSERT_EQ("large_list<item: large_list<item: string>>", lt2.ToString());
}

TEST(TestMapType, Basics) {
  std::shared_ptr<DataType> kt = std::make_shared<StringType>();
  std::shared_ptr<DataType> it = std::make_shared<UInt8Type>();

  MapType map_type(kt, it);
  ASSERT_EQ(map_type.id(), Type::MAP);

  ASSERT_EQ("map", map_type.name());
  ASSERT_EQ("map<string, uint8>", map_type.ToString());

  ASSERT_EQ(map_type.key_type()->id(), kt->id());
  ASSERT_EQ(map_type.item_type()->id(), it->id());
  ASSERT_EQ(map_type.value_type()->id(), Type::STRUCT);

  std::shared_ptr<DataType> mt = std::make_shared<MapType>(it, kt);
  ASSERT_EQ("map<uint8, string>", mt->ToString());

  MapType mt2(kt, mt, true);
  ASSERT_EQ("map<string, map<uint8, string>, keys_sorted>", mt2.ToString());
}

TEST(TestFixedSizeListType, Basics) {
  std::shared_ptr<DataType> vt = std::make_shared<UInt8Type>();

  FixedSizeListType fixed_size_list_type(vt, 4);
  ASSERT_EQ(fixed_size_list_type.id(), Type::FIXED_SIZE_LIST);

  ASSERT_EQ(4, fixed_size_list_type.list_size());
  ASSERT_EQ("fixed_size_list", fixed_size_list_type.name());
  ASSERT_EQ("fixed_size_list<item: uint8>[4]", fixed_size_list_type.ToString());

  ASSERT_EQ(fixed_size_list_type.value_type()->id(), vt->id());
  ASSERT_EQ(fixed_size_list_type.value_type()->id(), vt->id());

  std::shared_ptr<DataType> st = std::make_shared<StringType>();
  std::shared_ptr<DataType> lt = std::make_shared<FixedSizeListType>(st, 3);
  ASSERT_EQ("fixed_size_list<item: string>[3]", lt->ToString());

  FixedSizeListType lt2(lt, 7);
  ASSERT_EQ("fixed_size_list<item: fixed_size_list<item: string>[3]>[7]", lt2.ToString());
}

TEST(TestDateTypes, Attrs) {
  auto t1 = date32();
  auto t2 = date64();

  ASSERT_EQ("date32[day]", t1->ToString());
  ASSERT_EQ("date64[ms]", t2->ToString());

  ASSERT_EQ(32, checked_cast<const FixedWidthType&>(*t1).bit_width());
  ASSERT_EQ(64, checked_cast<const FixedWidthType&>(*t2).bit_width());
}

TEST(TestTimeType, Equals) {
  Time32Type t0;
  Time32Type t1(TimeUnit::SECOND);
  Time32Type t2(TimeUnit::MILLI);
  Time64Type t3(TimeUnit::MICRO);
  Time64Type t4(TimeUnit::NANO);
  Time64Type t5(TimeUnit::MICRO);

  ASSERT_EQ(32, t0.bit_width());
  ASSERT_EQ(64, t3.bit_width());

  AssertTypesEqual(t0, t2);
  AssertTypesEqual(t1, t1);
  AssertTypesNotEqual(t1, t3);
  AssertTypesNotEqual(t3, t4);
  AssertTypesEqual(t3, t5);
}

TEST(TestTimeType, ToString) {
  auto t1 = time32(TimeUnit::MILLI);
  auto t2 = time64(TimeUnit::NANO);
  auto t3 = time32(TimeUnit::SECOND);
  auto t4 = time64(TimeUnit::MICRO);

  ASSERT_EQ("time32[ms]", t1->ToString());
  ASSERT_EQ("time64[ns]", t2->ToString());
  ASSERT_EQ("time32[s]", t3->ToString());
  ASSERT_EQ("time64[us]", t4->ToString());
}

TEST(TestMonthIntervalType, Equals) {
  MonthIntervalType t1;
  MonthIntervalType t2;
  DayTimeIntervalType t3;

  AssertTypesEqual(t1, t2);
  AssertTypesNotEqual(t1, t3);
}

TEST(TestMonthIntervalType, ToString) {
  auto t1 = month_interval();

  ASSERT_EQ("month_interval", t1->ToString());
}

TEST(TestDayTimeIntervalType, Equals) {
  DayTimeIntervalType t1;
  DayTimeIntervalType t2;
  MonthIntervalType t3;

  AssertTypesEqual(t1, t2);
  AssertTypesNotEqual(t1, t3);
}

TEST(TestDayTimeIntervalType, ToString) {
  auto t1 = day_time_interval();

  ASSERT_EQ("day_time_interval", t1->ToString());
}

TEST(TestDurationType, Equals) {
  DurationType t1;
  DurationType t2;
  DurationType t3(TimeUnit::NANO);
  DurationType t4(TimeUnit::NANO);

  AssertTypesEqual(t1, t2);
  AssertTypesNotEqual(t1, t3);
  AssertTypesEqual(t3, t4);
}

TEST(TestDurationType, ToString) {
  auto t1 = duration(TimeUnit::MILLI);
  auto t2 = duration(TimeUnit::NANO);
  auto t3 = duration(TimeUnit::SECOND);
  auto t4 = duration(TimeUnit::MICRO);

  ASSERT_EQ("duration[ms]", t1->ToString());
  ASSERT_EQ("duration[ns]", t2->ToString());
  ASSERT_EQ("duration[s]", t3->ToString());
  ASSERT_EQ("duration[us]", t4->ToString());
}

TEST(TestTimestampType, Equals) {
  TimestampType t1;
  TimestampType t2;
  TimestampType t3(TimeUnit::NANO);
  TimestampType t4(TimeUnit::NANO);

  DurationType dt1;
  DurationType dt2(TimeUnit::NANO);

  AssertTypesEqual(t1, t2);
  AssertTypesNotEqual(t1, t3);
  AssertTypesEqual(t3, t4);

  AssertTypesNotEqual(t1, dt1);
  AssertTypesNotEqual(t3, dt2);
}

TEST(TestTimestampType, ToString) {
  auto t1 = timestamp(TimeUnit::MILLI);
  auto t2 = timestamp(TimeUnit::NANO, "US/Eastern");
  auto t3 = timestamp(TimeUnit::SECOND);
  auto t4 = timestamp(TimeUnit::MICRO);

  ASSERT_EQ("timestamp[ms]", t1->ToString());
  ASSERT_EQ("timestamp[ns, tz=US/Eastern]", t2->ToString());
  ASSERT_EQ("timestamp[s]", t3->ToString());
  ASSERT_EQ("timestamp[us]", t4->ToString());
}

TEST(TestListType, Equals) {
  auto t1 = list(utf8());
  auto t2 = list(utf8());
  auto t3 = list(binary());
  auto t4 = large_list(binary());
  auto t5 = large_list(binary());
  auto t6 = large_list(float64());

  AssertTypesEqual(*t1, *t2);
  AssertTypesNotEqual(*t1, *t3);
  AssertTypesNotEqual(*t3, *t4);
  AssertTypesEqual(*t4, *t5);
  AssertTypesNotEqual(*t5, *t6);
}

TEST(TestListType, Metadata) {
  auto md1 = key_value_metadata({"foo", "bar"}, {"foo value", "bar value"});
  auto md2 = key_value_metadata({"foo", "bar"}, {"foo value", "bar value"});
  auto md3 = key_value_metadata({"foo"}, {"foo value"});

  auto f1 = field("item", utf8(), /*nullable =*/true, md1);
  auto f2 = field("item", utf8(), /*nullable =*/true, md2);
  auto f3 = field("item", utf8(), /*nullable =*/true, md3);
  auto f4 = field("item", utf8());
  auto f5 = field("item", utf8(), /*nullable =*/false, md1);

  auto t1 = list(f1);
  auto t2 = list(f2);
  auto t3 = list(f3);
  auto t4 = list(f4);
  auto t5 = list(f5);

  AssertTypesEqual(*t1, *t2);
  AssertTypesNotEqual(*t1, *t3);
  AssertTypesNotEqual(*t1, *t4);
  AssertTypesNotEqual(*t1, *t5);

  AssertTypesEqual(*t1, *t2, /*check_metadata =*/false);
  AssertTypesEqual(*t1, *t3, /*check_metadata =*/false);
  AssertTypesEqual(*t1, *t4, /*check_metadata =*/false);
  AssertTypesNotEqual(*t1, *t5, /*check_metadata =*/false);
}

TEST(TestNestedType, Equals) {
  auto create_struct = [](std::string inner_name,
                          std::string struct_name) -> std::shared_ptr<Field> {
    auto f_type = field(inner_name, int32());
    std::vector<std::shared_ptr<Field>> fields = {f_type};
    auto s_type = std::make_shared<StructType>(fields);
    return field(struct_name, s_type);
  };

  auto create_union = [](std::string inner_name,
                         std::string union_name) -> std::shared_ptr<Field> {
    auto f_type = field(inner_name, int32());
    std::vector<std::shared_ptr<Field>> fields = {f_type};
    std::vector<uint8_t> codes = {Type::INT32};
    auto u_type = std::make_shared<UnionType>(fields, codes, UnionMode::SPARSE);
    return field(union_name, u_type);
  };

  auto s0 = create_struct("f0", "s0");
  auto s0_other = create_struct("f0", "s0");
  auto s0_bad = create_struct("f1", "s0");
  auto s1 = create_struct("f1", "s1");

  AssertFieldsEqual(*s0, *s0_other);
  AssertFieldsNotEqual(*s0, *s1);
  AssertFieldsNotEqual(*s0, *s0_bad);

  auto u0 = create_union("f0", "u0");
  auto u0_other = create_union("f0", "u0");
  auto u0_bad = create_union("f1", "u0");
  auto u1 = create_union("f1", "u1");

  AssertFieldsEqual(*u0, *u0_other);
  AssertFieldsNotEqual(*u0, *u1);
  AssertFieldsNotEqual(*u0, *u0_bad);
}

TEST(TestStructType, Basics) {
  auto f0_type = int32();
  auto f0 = field("f0", f0_type);

  auto f1_type = utf8();
  auto f1 = field("f1", f1_type);

  auto f2_type = uint8();
  auto f2 = field("f2", f2_type);

  std::vector<std::shared_ptr<Field>> fields = {f0, f1, f2};

  StructType struct_type(fields);

  ASSERT_TRUE(struct_type.child(0)->Equals(f0));
  ASSERT_TRUE(struct_type.child(1)->Equals(f1));
  ASSERT_TRUE(struct_type.child(2)->Equals(f2));

  ASSERT_EQ(struct_type.ToString(), "struct<f0: int32, f1: string, f2: uint8>");

  // TODO(wesm): out of bounds for field(...)
}

TEST(TestStructType, GetFieldByName) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", list(int16()));

  StructType struct_type({f0, f1, f2, f3});
  std::shared_ptr<Field> result;

  result = struct_type.GetFieldByName("f1");
  ASSERT_EQ(f1, result);

  result = struct_type.GetFieldByName("f3");
  ASSERT_EQ(f3, result);

  result = struct_type.GetFieldByName("not-found");
  ASSERT_EQ(result, nullptr);
}

TEST(TestStructType, GetFieldIndex) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", uint8(), false);
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", list(int16()));

  StructType struct_type({f0, f1, f2, f3});

  ASSERT_EQ(0, struct_type.GetFieldIndex(f0->name()));
  ASSERT_EQ(1, struct_type.GetFieldIndex(f1->name()));
  ASSERT_EQ(2, struct_type.GetFieldIndex(f2->name()));
  ASSERT_EQ(3, struct_type.GetFieldIndex(f3->name()));
  ASSERT_EQ(-1, struct_type.GetFieldIndex("not-found"));
}

TEST(TestStructType, GetFieldDuplicates) {
  auto f0 = field("f0", int32());
  auto f1 = field("f1", int64());
  auto f2 = field("f1", utf8());
  StructType struct_type({f0, f1, f2});

  ASSERT_EQ(0, struct_type.GetFieldIndex("f0"));
  ASSERT_EQ(-1, struct_type.GetFieldIndex("f1"));
  ASSERT_EQ(std::vector<int>{0}, struct_type.GetAllFieldIndices(f0->name()));
  AssertSortedEquals(std::vector<int>{1, 2}, struct_type.GetAllFieldIndices(f1->name()));

  std::vector<std::shared_ptr<Field>> results;

  results = struct_type.GetAllFieldsByName(f0->name());
  ASSERT_EQ(results.size(), 1);
  ASSERT_TRUE(results[0]->Equals(f0));

  results = struct_type.GetAllFieldsByName(f1->name());
  ASSERT_EQ(results.size(), 2);
  if (results[0]->type()->id() == Type::INT64) {
    ASSERT_TRUE(results[0]->Equals(f1));
    ASSERT_TRUE(results[1]->Equals(f2));
  } else {
    ASSERT_TRUE(results[0]->Equals(f2));
    ASSERT_TRUE(results[1]->Equals(f1));
  }

  results = struct_type.GetAllFieldsByName("not-found");
  ASSERT_EQ(results.size(), 0);
}

TEST(TestDictionaryType, Basics) {
  auto value_type = int32();

  std::shared_ptr<DictionaryType> type1 =
      std::dynamic_pointer_cast<DictionaryType>(dictionary(int16(), value_type));

  auto type2 = std::dynamic_pointer_cast<DictionaryType>(
      ::arrow::dictionary(int16(), type1, true));

  ASSERT_TRUE(int16()->Equals(type1->index_type()));
  ASSERT_TRUE(type1->value_type()->Equals(value_type));

  ASSERT_TRUE(int16()->Equals(type2->index_type()));
  ASSERT_TRUE(type2->value_type()->Equals(type1));

  ASSERT_EQ("dictionary<values=int32, indices=int16, ordered=0>", type1->ToString());
  ASSERT_EQ(
      "dictionary<values="
      "dictionary<values=int32, indices=int16, ordered=0>, "
      "indices=int16, ordered=1>",
      type2->ToString());
}

TEST(TestDictionaryType, Equals) {
  auto t1 = dictionary(int8(), int32());
  auto t2 = dictionary(int8(), int32());
  auto t3 = dictionary(int16(), int32());
  auto t4 = dictionary(int8(), int16());

  AssertTypesEqual(*t1, *t2);
  AssertTypesNotEqual(*t1, *t3);
  AssertTypesNotEqual(*t1, *t4);

  auto t5 = dictionary(int8(), int32(), /*ordered=*/false);
  auto t6 = dictionary(int8(), int32(), /*ordered=*/true);
  AssertTypesNotEqual(*t5, *t6);
}

void CheckTransposeMap(const Buffer& map, std::vector<int32_t> expected) {
  AssertBufferEqual(map, *Buffer::Wrap(expected));
}

TEST(TestDictionaryType, UnifyNumeric) {
  auto dict_ty = int64();

  auto t1 = dictionary(int8(), dict_ty);
  auto d1 = ArrayFromJSON(dict_ty, "[3, 4, 7]");

  auto t2 = dictionary(int8(), dict_ty);
  auto d2 = ArrayFromJSON(dict_ty, "[1, 7, 4, 8]");

  auto t3 = dictionary(int8(), dict_ty);
  auto d3 = ArrayFromJSON(dict_ty, "[1, -200]");

  auto expected = dictionary(int8(), dict_ty);
  auto expected_dict = ArrayFromJSON(dict_ty, "[3, 4, 7, 1, 8, -200]");

  std::unique_ptr<DictionaryUnifier> unifier;
  ASSERT_OK(DictionaryUnifier::Make(default_memory_pool(), dict_ty, &unifier));

  std::shared_ptr<DataType> out_type;
  std::shared_ptr<Array> out_dict;

  ASSERT_OK(unifier->Unify(*d1));
  ASSERT_OK(unifier->Unify(*d2));
  ASSERT_OK(unifier->Unify(*d3));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));

  std::shared_ptr<Buffer> b1, b2, b3;

  ASSERT_OK(unifier->Unify(*d1, &b1));
  ASSERT_OK(unifier->Unify(*d2, &b2));
  ASSERT_OK(unifier->Unify(*d3, &b3));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));

  CheckTransposeMap(*b1, {0, 1, 2});
  CheckTransposeMap(*b2, {3, 2, 1, 4});
  CheckTransposeMap(*b3, {3, 5});
}

TEST(TestDictionaryType, UnifyString) {
  auto dict_ty = utf8();

  auto t1 = dictionary(int16(), dict_ty);
  auto d1 = ArrayFromJSON(dict_ty, "[\"foo\", \"bar\"]");

  auto t2 = dictionary(int32(), dict_ty);
  auto d2 = ArrayFromJSON(dict_ty, "[\"quux\", \"foo\"]");

  auto expected = dictionary(int8(), dict_ty);
  auto expected_dict = ArrayFromJSON(dict_ty, "[\"foo\", \"bar\", \"quux\"]");

  std::unique_ptr<DictionaryUnifier> unifier;
  ASSERT_OK(DictionaryUnifier::Make(default_memory_pool(), dict_ty, &unifier));

  std::shared_ptr<DataType> out_type;
  std::shared_ptr<Array> out_dict;
  ASSERT_OK(unifier->Unify(*d1));
  ASSERT_OK(unifier->Unify(*d2));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));

  std::shared_ptr<Buffer> b1, b2;

  ASSERT_OK(unifier->Unify(*d1, &b1));
  ASSERT_OK(unifier->Unify(*d2, &b2));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));

  CheckTransposeMap(*b1, {0, 1});
  CheckTransposeMap(*b2, {2, 0});
}

TEST(TestDictionaryType, UnifyFixedSizeBinary) {
  auto type = fixed_size_binary(3);

  std::string data = "foobarbazqux";
  auto buf = std::make_shared<Buffer>(data);
  // ["foo", "bar"]
  auto dict1 = std::make_shared<FixedSizeBinaryArray>(type, 2, SliceBuffer(buf, 0, 6));
  auto t1 = dictionary(int16(), type);
  // ["bar", "baz", "qux"]
  auto dict2 = std::make_shared<FixedSizeBinaryArray>(type, 3, SliceBuffer(buf, 3, 9));
  auto t2 = dictionary(int16(), type);

  // ["foo", "bar", "baz", "qux"]
  auto expected_dict = std::make_shared<FixedSizeBinaryArray>(type, 4, buf);
  auto expected = dictionary(int8(), type);

  std::unique_ptr<DictionaryUnifier> unifier;
  ASSERT_OK(DictionaryUnifier::Make(default_memory_pool(), type, &unifier));
  std::shared_ptr<DataType> out_type;
  std::shared_ptr<Array> out_dict;
  ASSERT_OK(unifier->Unify(*dict1));
  ASSERT_OK(unifier->Unify(*dict2));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));

  std::shared_ptr<Buffer> b1, b2;
  ASSERT_OK(unifier->Unify(*dict1, &b1));
  ASSERT_OK(unifier->Unify(*dict2, &b2));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));

  CheckTransposeMap(*b1, {0, 1});
  CheckTransposeMap(*b2, {1, 2, 3});
}

TEST(TestDictionaryType, UnifyLarge) {
  // Unifying "large" dictionary types should choose the right index type
  std::shared_ptr<Array> dict1, dict2, expected_dict;

  Int32Builder builder;
  ASSERT_OK(builder.Reserve(120));
  for (int32_t i = 0; i < 120; ++i) {
    builder.UnsafeAppend(i);
  }
  ASSERT_OK(builder.Finish(&dict1));
  ASSERT_EQ(dict1->length(), 120);
  auto t1 = dictionary(int8(), int32());

  ASSERT_OK(builder.Reserve(30));
  for (int32_t i = 110; i < 140; ++i) {
    builder.UnsafeAppend(i);
  }
  ASSERT_OK(builder.Finish(&dict2));
  ASSERT_EQ(dict2->length(), 30);
  auto t2 = dictionary(int8(), int32());

  ASSERT_OK(builder.Reserve(140));
  for (int32_t i = 0; i < 140; ++i) {
    builder.UnsafeAppend(i);
  }
  ASSERT_OK(builder.Finish(&expected_dict));
  ASSERT_EQ(expected_dict->length(), 140);

  // int8 would be too narrow to hold all possible index values
  auto expected = dictionary(int16(), int32());

  std::unique_ptr<DictionaryUnifier> unifier;
  ASSERT_OK(DictionaryUnifier::Make(default_memory_pool(), int32(), &unifier));
  std::shared_ptr<DataType> out_type;
  std::shared_ptr<Array> out_dict;
  ASSERT_OK(unifier->Unify(*dict1));
  ASSERT_OK(unifier->Unify(*dict2));
  ASSERT_OK(unifier->GetResult(&out_type, &out_dict));
  ASSERT_TRUE(out_type->Equals(*expected));
  ASSERT_TRUE(out_dict->Equals(*expected_dict));
}

TEST(TypesTest, TestDecimal128Small) {
  Decimal128Type t1(8, 4);

  ASSERT_EQ(t1.id(), Type::DECIMAL);
  ASSERT_EQ(t1.precision(), 8);
  ASSERT_EQ(t1.scale(), 4);

  ASSERT_EQ(t1.ToString(), std::string("decimal(8, 4)"));

  // Test properties
  ASSERT_EQ(t1.byte_width(), 16);
  ASSERT_EQ(t1.bit_width(), 128);
}

TEST(TypesTest, TestDecimal128Medium) {
  Decimal128Type t1(12, 5);

  ASSERT_EQ(t1.id(), Type::DECIMAL);
  ASSERT_EQ(t1.precision(), 12);
  ASSERT_EQ(t1.scale(), 5);

  ASSERT_EQ(t1.ToString(), std::string("decimal(12, 5)"));

  // Test properties
  ASSERT_EQ(t1.byte_width(), 16);
  ASSERT_EQ(t1.bit_width(), 128);
}

TEST(TypesTest, TestDecimal128Large) {
  Decimal128Type t1(27, 7);

  ASSERT_EQ(t1.id(), Type::DECIMAL);
  ASSERT_EQ(t1.precision(), 27);
  ASSERT_EQ(t1.scale(), 7);

  ASSERT_EQ(t1.ToString(), std::string("decimal(27, 7)"));

  // Test properties
  ASSERT_EQ(t1.byte_width(), 16);
  ASSERT_EQ(t1.bit_width(), 128);
}

TEST(TypesTest, TestDecimalEquals) {
  Decimal128Type t1(8, 4);
  Decimal128Type t2(8, 4);
  Decimal128Type t3(8, 5);
  Decimal128Type t4(27, 5);

  FixedSizeBinaryType t9(16);

  AssertTypesEqual(t1, t2);
  AssertTypesNotEqual(t1, t3);
  AssertTypesNotEqual(t1, t4);
  AssertTypesNotEqual(t1, t9);
}

}  // namespace arrow
