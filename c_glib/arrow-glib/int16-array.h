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

#define GARROW_TYPE_INT16_ARRAY                  \
  (garrow_int16_array_get_type())
#define GARROW_INT16_ARRAY(obj)                          \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),                    \
                              GARROW_TYPE_INT16_ARRAY,   \
                              GArrowInt16Array))
#define GARROW_INT16_ARRAY_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_CAST((klass),                     \
                           GARROW_TYPE_INT16_ARRAY,      \
                           GArrowInt16ArrayClass))
#define GARROW_IS_INT16_ARRAY(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),                    \
                              GARROW_TYPE_INT16_ARRAY))
#define GARROW_IS_INT16_ARRAY_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                     \
                           GARROW_TYPE_INT16_ARRAY))
#define GARROW_INT16_ARRAY_GET_CLASS(obj)                \
  (G_TYPE_INSTANCE_GET_CLASS((obj),                     \
                             GARROW_TYPE_INT16_ARRAY,    \
                             GArrowInt16ArrayClass))

typedef struct _GArrowInt16Array         GArrowInt16Array;
typedef struct _GArrowInt16ArrayClass    GArrowInt16ArrayClass;

/**
 * GArrowInt16Array:
 *
 * It wraps `arrow::Int16Array`.
 */
struct _GArrowInt16Array
{
  /*< private >*/
  GArrowArray parent_instance;
};

struct _GArrowInt16ArrayClass
{
  GArrowArrayClass parent_class;
};

GType garrow_int16_array_get_type(void) G_GNUC_CONST;

gint16 garrow_int16_array_get_value(GArrowInt16Array *array,
                                  gint64 i);

G_END_DECLS
