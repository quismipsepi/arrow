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

// Functions for converting between CPython built-in data structures and Arrow
// data structures

#ifndef PYARROW_ADAPTERS_BUILTIN_H
#define PYARROW_ADAPTERS_BUILTIN_H

#include <Python.h>

#include <memory>

#include <arrow/type.h>

#include "pyarrow/common.h"
#include "pyarrow/visibility.h"

namespace arrow {
class Array;
class Status;
}

namespace pyarrow {

PYARROW_EXPORT arrow::Status InferArrowType(
    PyObject* obj, int64_t* size, std::shared_ptr<arrow::DataType>* out_type);

PYARROW_EXPORT arrow::Status AppendPySequence(PyObject* obj,
    const std::shared_ptr<arrow::DataType>& type,
    const std::shared_ptr<arrow::ArrayBuilder>& builder);

PYARROW_EXPORT
arrow::Status ConvertPySequence(
    PyObject* obj, arrow::MemoryPool* pool, std::shared_ptr<arrow::Array>* out);

}  // namespace pyarrow

#endif  // PYARROW_ADAPTERS_BUILTIN_H
