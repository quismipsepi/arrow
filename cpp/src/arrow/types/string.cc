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

#include "arrow/types/string.h"

#include <sstream>
#include <string>

#include "arrow/type.h"

namespace arrow {

const std::shared_ptr<DataType> STRING(new StringType());

StringArray::StringArray(int32_t length, const std::shared_ptr<Buffer>& offsets,
    const ArrayPtr& values, int32_t null_count,
    const std::shared_ptr<Buffer>& null_bitmap)
    : StringArray(STRING, length, offsets, values, null_count, null_bitmap) {}

std::string CharType::ToString() const {
  std::stringstream s;
  s << "char(" << size << ")";
  return s.str();
}

std::string VarcharType::ToString() const {
  std::stringstream s;
  s << "varchar(" << size << ")";
  return s.str();
}

TypePtr StringBuilder::value_type_ = TypePtr(new UInt8Type());

}  // namespace arrow
