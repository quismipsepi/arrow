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

#pragma once

#include <RcppCommon.h>

#undef Free
#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/io/memory.h>
#include <arrow/type.h>

#define R_ERROR_NOT_OK(s)                  \
  do {                                     \
    if (!s.ok()) Rcpp::stop(s.ToString()); \
  } while (0);

template <typename T>
struct NoDelete {
  inline void operator()(T* ptr){};
};

namespace Rcpp {
namespace traits {

struct wrap_type_shared_ptr_tag {};

template <typename T>
struct wrap_type_traits<std::shared_ptr<T>> {
  using wrap_category = wrap_type_shared_ptr_tag;
};

template <typename T>
class Exporter<std::shared_ptr<T>>;

}  // namespace traits
namespace internal {

template <typename T>
inline SEXP wrap_dispatch(const T& x, Rcpp::traits::wrap_type_shared_ptr_tag);

}  // namespace internal

}  // namespace Rcpp

#include <Rcpp.h>

RCPP_EXPOSED_ENUM_NODECL(arrow::Type::type)
RCPP_EXPOSED_ENUM_NODECL(arrow::DateUnit)
RCPP_EXPOSED_ENUM_NODECL(arrow::TimeUnit::type)
RCPP_EXPOSED_ENUM_NODECL(arrow::StatusCode)
RCPP_EXPOSED_ENUM_NODECL(arrow::io::FileMode::type)

namespace Rcpp {
namespace traits {

template <typename T>
class Exporter<std::shared_ptr<T>> {
 public:
  Exporter(SEXP self) : xp(extract_xp(self)) {}

  inline std::shared_ptr<T> get() { return *Rcpp::XPtr<std::shared_ptr<T>>(xp); }

 private:
  SEXP xp;

  SEXP extract_xp(SEXP self) {
    static SEXP symb_xp = Rf_install(".:xp:.");
    return Rf_findVarInFrame(self, symb_xp);
  }
};

}  // namespace traits

namespace internal {

template <typename T>
inline SEXP wrap_dispatch(const T& x, Rcpp::traits::wrap_type_shared_ptr_tag) {
  return Rcpp::XPtr<std::shared_ptr<typename T::element_type>>(
      new std::shared_ptr<typename T::element_type>(x));
}

}  // namespace internal

}  // namespace Rcpp

namespace Rcpp {
using IntegerVector_ = Rcpp::Vector<INTSXP, Rcpp::NoProtectStorage>;
using LogicalVector_ = Rcpp::Vector<LGLSXP, Rcpp::NoProtectStorage>;
using StringVector_ = Rcpp::Vector<STRSXP, Rcpp::NoProtectStorage>;
using CharacterVector_ = StringVector_;

template <int RTYPE>
inline typename Rcpp::Vector<RTYPE>::stored_type default_value() {
  return Rcpp::Vector<RTYPE>::get_na();
}
template <>
inline Rbyte default_value<RAWSXP>() {
  return 0;
}

}  // namespace Rcpp

SEXP ChunkedArray__as_vector(const std::shared_ptr<arrow::ChunkedArray>& chunked_array);
SEXP Array__as_vector(const std::shared_ptr<arrow::Array>& array);
std::shared_ptr<arrow::Array> Array__from_vector(SEXP x);
std::shared_ptr<arrow::RecordBatch> RecordBatch__from_dataframe(Rcpp::DataFrame tbl);

namespace arrow {
namespace r {

template <typename T>
inline const T* GetValuesSafely(const std::shared_ptr<ArrayData>& data, int i,
                                int64_t offset) {
  auto buffer = data->buffers[i];
  if (!buffer) {
    Rcpp::stop(tfm::format("invalid data in buffer %d", i));
  };
  return reinterpret_cast<const T*>(buffer->data()) + offset;
}

template <int RTYPE, typename Vec = Rcpp::Vector<RTYPE>>
class RBuffer : public Buffer {
 public:
  RBuffer(Vec vec)
      : Buffer(reinterpret_cast<const uint8_t*>(vec.begin()),
               vec.size() * sizeof(typename Vec::stored_type)),
        vec_(vec) {}

 private:
  // vec_ holds the memory
  Vec vec_;
};

}  // namespace r
}  // namespace arrow
