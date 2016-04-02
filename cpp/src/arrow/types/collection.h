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

#ifndef ARROW_TYPES_COLLECTION_H
#define ARROW_TYPES_COLLECTION_H

#include <string>
#include <vector>

#include "arrow/type.h"

namespace arrow {

template <Type::type T>
struct CollectionType : public DataType {
  std::vector<TypePtr> child_types_;

  CollectionType() : DataType(T) {}

  const TypePtr& child(int i) const { return child_types_[i]; }

  int num_children() const { return child_types_.size(); }
};

}  // namespace arrow

#endif  // ARROW_TYPES_COLLECTION_H
