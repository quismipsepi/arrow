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

#ifndef PYARROW_UTIL_DATETIME_H
#define PYARROW_UTIL_DATETIME_H

#include "arrow/python/platform.h"
#include <datetime.h>

namespace arrow {
namespace py {

static inline int64_t PyDate_to_ms(PyDateTime_Date* pydate) {
  struct tm date = {0};
  date.tm_year = PyDateTime_GET_YEAR(pydate) - 1900;
  date.tm_mon = PyDateTime_GET_MONTH(pydate) - 1;
  date.tm_mday = PyDateTime_GET_DAY(pydate);
  struct tm epoch = {0};
  epoch.tm_year = 70;
  epoch.tm_mday = 1;
#ifdef _MSC_VER
  // Milliseconds since the epoch
  const int64_t current_timestamp = static_cast<int64_t>(_mkgmtime64(&date));
  const int64_t epoch_timestamp = static_cast<int64_t>(_mkgmtime64(&epoch));
  return (current_timestamp - epoch_timestamp) * 1000LL;
#else
  return lrint(difftime(mktime(&date), mktime(&epoch)) * 1000);
#endif
}

static inline int32_t PyDate_to_days(PyDateTime_Date* pydate) {
  return static_cast<int32_t>(PyDate_to_ms(pydate) / 86400000LL);
}

}  // namespace py
}  // namespace arrow

#endif  // PYARROW_UTIL_DATETIME_H
