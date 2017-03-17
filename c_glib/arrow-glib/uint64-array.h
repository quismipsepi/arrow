/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <arrow-glib/array.h>

G_BEGIN_DECLS

#define GARROW_TYPE_UINT64_ARRAY                 \
  (garrow_uint64_array_get_type())
#define GARROW_UINT64_ARRAY(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),                    \
                              GARROW_TYPE_UINT64_ARRAY,  \
                              GArrowUInt64Array))
#define GARROW_UINT64_ARRAY_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),                     \
                           GARROW_TYPE_UINT64_ARRAY,     \
                           GArrowUInt64ArrayClass))
#define GARROW_IS_UINT64_ARRAY(obj)                      \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),                    \
                              GARROW_TYPE_UINT64_ARRAY))
#define GARROW_IS_UINT64_ARRAY_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                     \
                           GARROW_TYPE_UINT64_ARRAY))
#define GARROW_UINT64_ARRAY_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS((obj),                     \
                             GARROW_TYPE_UINT64_ARRAY,   \
                             GArrowUInt64ArrayClass))

typedef struct _GArrowUInt64Array         GArrowUInt64Array;
typedef struct _GArrowUInt64ArrayClass    GArrowUInt64ArrayClass;

/**
 * GArrowUInt64Array:
 *
 * It wraps `arrow::UInt64Array`.
 */
struct _GArrowUInt64Array
{
  /*< private >*/
  GArrowArray parent_instance;
};

struct _GArrowUInt64ArrayClass
{
  GArrowArrayClass parent_class;
};

GType garrow_uint64_array_get_type(void) G_GNUC_CONST;

guint64 garrow_uint64_array_get_value(GArrowUInt64Array *array,
                                    gint64 i);

G_END_DECLS
