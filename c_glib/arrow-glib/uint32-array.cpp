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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <arrow-glib/array.hpp>
#include <arrow-glib/uint32-array.h>

G_BEGIN_DECLS

/**
 * SECTION: uint32-array
 * @short_description: 32-bit unsigned integer array class
 *
 * #GArrowUInt32Array is a class for 32-bit unsigned integer array. It
 * can store zero or more 32-bit unsigned integer data.
 *
 * #GArrowUInt32Array is immutable. You need to use
 * #GArrowUInt32ArrayBuilder to create a new array.
 */

G_DEFINE_TYPE(GArrowUInt32Array,               \
              garrow_uint32_array,             \
              GARROW_TYPE_ARRAY)

static void
garrow_uint32_array_init(GArrowUInt32Array *object)
{
}

static void
garrow_uint32_array_class_init(GArrowUInt32ArrayClass *klass)
{
}

/**
 * garrow_uint32_array_get_value:
 * @array: A #GArrowUInt32Array.
 * @i: The index of the target value.
 *
 * Returns: The i-th value.
 */
guint32
garrow_uint32_array_get_value(GArrowUInt32Array *array,
                              gint64 i)
{
  auto arrow_array = garrow_array_get_raw(GARROW_ARRAY(array));
  return static_cast<arrow::UInt32Array *>(arrow_array.get())->Value(i);
}

G_END_DECLS
