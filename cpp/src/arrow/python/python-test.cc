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

#include "gtest/gtest.h"

#include <memory>

#include "arrow/python/platform.h"

#include "arrow/array.h"
#include "arrow/builder.h"
#include "arrow/table.h"
#include "arrow/test-util.h"

#include "arrow/python/builtin_convert.h"
#include "arrow/python/common.h"
#include "arrow/python/helpers.h"
#include "arrow/python/pandas_convert.h"

#include "arrow/util/decimal.h"

namespace arrow {
namespace py {

TEST(PyBuffer, InvalidInputObject) {
  PyBuffer buffer(Py_None);
}

TEST(DecimalTest, TestPythonDecimalToArrowDecimal128) {
  PyAcquireGIL lock;

  OwnedRef decimal;
  OwnedRef Decimal;
  ASSERT_OK(ImportModule("decimal", &decimal));
  ASSERT_NE(decimal.obj(), nullptr);

  ASSERT_OK(ImportFromModule(decimal, "Decimal", &Decimal));
  ASSERT_NE(Decimal.obj(), nullptr);

  std::string decimal_string("-39402950693754869342983");
  const char* format = "s#";
  auto c_string = decimal_string.c_str();
  ASSERT_NE(c_string, nullptr);

  auto c_string_size = decimal_string.size();
  ASSERT_GT(c_string_size, 0);
  OwnedRef pydecimal(PyObject_CallFunction(
      Decimal.obj(), const_cast<char*>(format), c_string, c_string_size));
  ASSERT_NE(pydecimal.obj(), nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  decimal::Decimal128 arrow_decimal;
  boost::multiprecision::int128_t boost_decimal(decimal_string);
  PyObject* obj = pydecimal.obj();
  ASSERT_OK(PythonDecimalToArrowDecimal(obj, &arrow_decimal));
  ASSERT_EQ(boost_decimal, arrow_decimal.value);
}

TEST(PandasConversionTest, TestObjectBlockWriteFails) {
  StringBuilder builder(default_memory_pool());
  const char value[] = {'\xf1', '\0'};

  for (int i = 0; i < 1000; ++i) {
    builder.Append(value, static_cast<int32_t>(strlen(value)));
  }

  std::shared_ptr<Array> arr;
  ASSERT_OK(builder.Finish(&arr));

  auto f1 = field("f1", utf8());
  auto f2 = field("f2", utf8());
  auto f3 = field("f3", utf8());
  std::vector<std::shared_ptr<Field>> fields = {f1, f2, f3};
  std::vector<std::shared_ptr<Column>> cols = {std::make_shared<Column>(f1, arr),
      std::make_shared<Column>(f2, arr), std::make_shared<Column>(f3, arr)};

  auto schema = std::make_shared<Schema>(fields);
  auto table = std::make_shared<Table>(schema, cols);

  PyObject* out;
  Py_BEGIN_ALLOW_THREADS;
  ASSERT_RAISES(UnknownError, ConvertTableToPandas(table, 2, &out));
  Py_END_ALLOW_THREADS;
}

TEST(BuiltinConversionTest, TestMixedTypeFails) {
  PyAcquireGIL lock;
  MemoryPool* pool = default_memory_pool();
  std::shared_ptr<Array> arr;

  OwnedRef list_ref(PyList_New(3));
  PyObject* list = list_ref.obj();

  ASSERT_NE(list, nullptr);

  PyObject* str = PyUnicode_FromString("abc");
  ASSERT_NE(str, nullptr);

  PyObject* integer = PyLong_FromLong(1234L);
  ASSERT_NE(integer, nullptr);

  PyObject* doub = PyFloat_FromDouble(123.0234);
  ASSERT_NE(doub, nullptr);

  // This steals a reference to each object, so we don't need to decref them later
  // just the list
  ASSERT_EQ(PyList_SetItem(list, 0, str), 0);
  ASSERT_EQ(PyList_SetItem(list, 1, integer), 0);
  ASSERT_EQ(PyList_SetItem(list, 2, doub), 0);

  ASSERT_RAISES(UnknownError, ConvertPySequence(list, pool, &arr));
}

}  // namespace py
}  // namespace arrow
