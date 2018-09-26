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

#include "arrow_types.h"

using namespace Rcpp;

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Int8__initialize() { return arrow::int8(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Int16__initialize() { return arrow::int16(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Int32__initialize() { return arrow::int32(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Int64__initialize() { return arrow::int64(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> UInt8__initialize() { return arrow::uint8(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> UInt16__initialize() { return arrow::uint16(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> UInt32__initialize() { return arrow::uint32(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> UInt64__initialize() { return arrow::uint64(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Float16__initialize() { return arrow::float16(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Float32__initialize() { return arrow::float32(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Float64__initialize() { return arrow::float64(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Boolean__initialize() { return arrow::boolean(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Utf8__initialize() { return arrow::utf8(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Date32__initialize() { return arrow::date32(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Date64__initialize() { return arrow::date64(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Null__initialize() { return arrow::null(); }

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Decimal128Type__initialize(int32_t precision,
                                                            int32_t scale) {
  return arrow::decimal(precision, scale);
}

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> FixedSizeBinary__initialize(int32_t byte_width) {
  return arrow::fixed_size_binary(byte_width);
}

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Timestamp__initialize1(arrow::TimeUnit::type unit) {
  return arrow::timestamp(unit);
}

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Timestamp__initialize2(arrow::TimeUnit::type unit,
                                                        const std::string& timezone) {
  return arrow::timestamp(unit, timezone);
}

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Time32__initialize(arrow::TimeUnit::type unit) {
  return arrow::time32(unit);
}

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> Time64__initialize(arrow::TimeUnit::type unit) {
  return arrow::time64(unit);
}

// [[Rcpp::export]]
SEXP list__(SEXP x) {
  if (Rf_inherits(x, "arrow::Field")) {
    return wrap(arrow::list(Rcpp::as<std::shared_ptr<arrow::Field>>(x)));
  }

  if (Rf_inherits(x, "arrow::DataType")) {
    return wrap(arrow::list(Rcpp::as<std::shared_ptr<arrow::DataType>>(x)));
  }

  stop("incompatible");
  return R_NilValue;
}

template <typename T>
std::vector<std::shared_ptr<T>> List_to_shared_ptr_vector(List x) {
  int n = x.size();
  std::vector<std::shared_ptr<T>> vec;
  for (SEXP element : x) {
    vec.push_back(as<std::shared_ptr<T>>(element));
  }
  return vec;
}

// [[Rcpp::export]]
std::shared_ptr<arrow::DataType> struct_(List fields) {
  return arrow::struct_(List_to_shared_ptr_vector<arrow::Field>(fields));
}

// [[Rcpp::export]]
std::string DataType__ToString(const std::shared_ptr<arrow::DataType>& type) {
  return type->ToString();
}

// [[Rcpp::export]]
std::string DataType__name(const std::shared_ptr<arrow::DataType>& type) {
  return type->name();
}

// [[Rcpp::export]]
bool DataType__Equals(const std::shared_ptr<arrow::DataType>& lhs,
                      const std::shared_ptr<arrow::DataType>& rhs) {
  return lhs->Equals(*rhs);
}

// [[Rcpp::export]]
int DataType__num_children(const std::shared_ptr<arrow::DataType>& type) {
  return type->num_children();
}

// [[Rcpp::export]]
List DataType__children_pointer(const std::shared_ptr<arrow::DataType>& type) {
  return List(type->children().begin(), type->children().end());
}

// [[Rcpp::export]]
arrow::Type::type DataType__id(const std::shared_ptr<arrow::DataType>& type) {
  return type->id();
}

// [[Rcpp::export]]
std::shared_ptr<arrow::Schema> schema_(List fields) {
  return arrow::schema(List_to_shared_ptr_vector<arrow::Field>(fields));
}

// [[Rcpp::export]]
std::string Schema__ToString(const std::shared_ptr<arrow::Schema>& s) {
  return s->ToString();
}

// [[Rcpp::export]]
std::string ListType__ToString(const std::shared_ptr<arrow::ListType>& type) {
  return type->ToString();
}

// [[Rcpp::export]]
int FixedWidthType__bit_width(const std::shared_ptr<arrow::FixedWidthType>& type) {
  return type->bit_width();
}

// [[Rcpp::export]]
arrow::DateUnit DateType__unit(const std::shared_ptr<arrow::DateType>& type) {
  return type->unit();
}

// [[Rcpp::export]]
arrow::TimeUnit::type TimeType__unit(const std::shared_ptr<arrow::TimeType>& type) {
  return type->unit();
}

// [[Rcpp::export]]
int32_t DecimalType__precision(const std::shared_ptr<arrow::DecimalType>& type) {
  return type->precision();
}

// [[Rcpp::export]]
int32_t DecimalType__scale(const std::shared_ptr<arrow::DecimalType>& type) {
  return type->scale();
}

// [[Rcpp::export]]
std::string TimestampType__timezone(const std::shared_ptr<arrow::TimestampType>& type) {
  return type->timezone();
}

// [[Rcpp::export]]
arrow::TimeUnit::type TimestampType__unit(
    const std::shared_ptr<arrow::TimestampType>& type) {
  return type->unit();
}

// [[Rcpp::export]]
std::string Object__pointer_address(SEXP obj) {
  return tfm::format("%p", EXTPTR_PTR(obj));
}
