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

#include <ostream>
#include <string>

#include "arrow/array.h"
#include "arrow/pretty_print.h"
#include "arrow/table.h"
#include "arrow/type.h"
#include "arrow/type_traits.h"
#include "arrow/types/list.h"
#include "arrow/types/string.h"
#include "arrow/types/struct.h"
#include "arrow/util/status.h"

namespace arrow {

class ArrayPrinter : public ArrayVisitor {
 public:
  ArrayPrinter(const Array& array, std::ostream* sink) : array_(array), sink_(sink) {}

  Status Print() { return VisitArray(array_); }

  Status VisitArray(const Array& array) { return array.Accept(this); }

  template <typename T>
  typename std::enable_if<IsNumeric<T>::value, void>::type WriteDataValues(
      const T& array) {
    const auto data = array.raw_data();
    for (int i = 0; i < array.length(); ++i) {
      if (i > 0) { (*sink_) << ", "; }
      if (array.IsNull(i)) {
        (*sink_) << "null";
      } else {
        (*sink_) << data[i];
      }
    }
  }

  // String (Utf8), Binary
  template <typename T>
  typename std::enable_if<std::is_base_of<BinaryArray, T>::value, void>::type
  WriteDataValues(const T& array) {
    int32_t length;
    for (int i = 0; i < array.length(); ++i) {
      if (i > 0) { (*sink_) << ", "; }
      if (array.IsNull(i)) {
        (*sink_) << "null";
      } else {
        const char* buf = reinterpret_cast<const char*>(array.GetValue(i, &length));
        (*sink_) << "\"" << std::string(buf, length) << "\"";
      }
    }
  }

  template <typename T>
  typename std::enable_if<std::is_base_of<BooleanArray, T>::value, void>::type
  WriteDataValues(const T& array) {
    for (int i = 0; i < array.length(); ++i) {
      if (i > 0) { (*sink_) << ", "; }
      if (array.IsNull(i)) {
        (*sink_) << "null";
      } else {
        (*sink_) << (array.Value(i) ? "true" : "false");
      }
    }
  }

  void OpenArray() { (*sink_) << "["; }

  void CloseArray() { (*sink_) << "]"; }

  template <typename T>
  Status WritePrimitive(const T& array) {
    OpenArray();
    WriteDataValues(array);
    CloseArray();
    return Status::OK();
  }

  template <typename T>
  Status WriteVarBytes(const T& array) {
    OpenArray();
    WriteDataValues(array);
    CloseArray();
    return Status::OK();
  }

  Status Visit(const NullArray& array) override { return Status::OK(); }

  Status Visit(const BooleanArray& array) override { return WritePrimitive(array); }

  Status Visit(const Int8Array& array) override { return WritePrimitive(array); }

  Status Visit(const Int16Array& array) override { return WritePrimitive(array); }

  Status Visit(const Int32Array& array) override { return WritePrimitive(array); }

  Status Visit(const Int64Array& array) override { return WritePrimitive(array); }

  Status Visit(const UInt8Array& array) override { return WritePrimitive(array); }

  Status Visit(const UInt16Array& array) override { return WritePrimitive(array); }

  Status Visit(const UInt32Array& array) override { return WritePrimitive(array); }

  Status Visit(const UInt64Array& array) override { return WritePrimitive(array); }

  Status Visit(const HalfFloatArray& array) override { return WritePrimitive(array); }

  Status Visit(const FloatArray& array) override { return WritePrimitive(array); }

  Status Visit(const DoubleArray& array) override { return WritePrimitive(array); }

  Status Visit(const StringArray& array) override { return WriteVarBytes(array); }

  Status Visit(const BinaryArray& array) override { return WriteVarBytes(array); }

  Status Visit(const DateArray& array) override { return Status::NotImplemented("date"); }

  Status Visit(const TimeArray& array) override { return Status::NotImplemented("time"); }

  Status Visit(const TimestampArray& array) override {
    return Status::NotImplemented("timestamp");
  }

  Status Visit(const IntervalArray& array) override {
    return Status::NotImplemented("interval");
  }

  Status Visit(const DecimalArray& array) override {
    return Status::NotImplemented("decimal");
  }

  Status Visit(const ListArray& array) override {
    // auto type = static_cast<const ListType*>(array.type().get());
    // for (size_t i = 0; i < fields.size(); ++i) {
    //   RETURN_NOT_OK(VisitArray(fields[i]->name, *arrays[i].get()));
    // }
    // return WriteChildren(type->children(), {array.values()});
    return Status::OK();
  }

  Status Visit(const StructArray& array) override {
    // auto type = static_cast<const StructType*>(array.type().get());
    // for (size_t i = 0; i < fields.size(); ++i) {
    //   RETURN_NOT_OK(VisitArray(fields[i]->name, *arrays[i].get()));
    // }
    // return WriteChildren(type->children(), array.fields());
    return Status::OK();
  }

  Status Visit(const UnionArray& array) override {
    return Status::NotImplemented("union");
  }

 private:
  const Array& array_;
  std::ostream* sink_;
};

Status PrettyPrint(const Array& arr, std::ostream* sink) {
  ArrayPrinter printer(arr, sink);
  return printer.Print();
}

Status PrettyPrint(const RecordBatch& batch, std::ostream* sink) {
  for (int i = 0; i < batch.num_columns(); ++i) {
    const std::string& name = batch.column_name(i);
    (*sink) << name << ": ";
    RETURN_NOT_OK(PrettyPrint(*batch.column(i).get(), sink));
    (*sink) << "\n";
  }
  return Status::OK();
}

}  // namespace arrow
