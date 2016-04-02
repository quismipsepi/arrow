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

#include "arrow/type.h"

#include <sstream>
#include <string>

namespace arrow {

std::string Field::ToString() const {
  std::stringstream ss;
  ss << this->name << ": " << this->type->ToString();
  if (!this->nullable) { ss << " not null"; }
  return ss.str();
}

DataType::~DataType() {}

StringType::StringType() : DataType(Type::STRING) {}

std::string StringType::ToString() const {
  std::string result(name());
  return result;
}

std::string ListType::ToString() const {
  std::stringstream s;
  s << "list<" << value_field()->ToString() << ">";
  return s.str();
}

std::string StructType::ToString() const {
  std::stringstream s;
  s << "struct<";
  for (int i = 0; i < this->num_children(); ++i) {
    if (i > 0) { s << ", "; }
    const std::shared_ptr<Field>& field = this->child(i);
    s << field->name << ": " << field->type->ToString();
  }
  s << ">";
  return s.str();
}

}  // namespace arrow
