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

#include <arrow-glib/array-builder.hpp>
#include <arrow-glib/data-type.hpp>
#include <arrow-glib/error.hpp>

G_BEGIN_DECLS

/**
 * SECTION: array-builder
 * @section_id: array-builder-classes
 * @title: Array builder classes
 * @include: arrow-glib/arrow-glib.h
 *
 * #GArrowArrayBuilder is a base class for all array builder classes
 * such as #GArrowBooleanArrayBuilder.
 *
 * You need to use array builder class to create a new array.
 *
 * #GArrowBooleanArrayBuilder is the class to create a new
 * #GArrowBooleanArray.
 *
 * #GArrowInt8ArrayBuilder is the class to create a new
 * #GArrowInt8Array.
 *
 * #GArrowUInt8ArrayBuilder is the class to create a new
 * #GArrowUInt8Array.
 *
 * #GArrowInt16ArrayBuilder is the class to create a new
 * #GArrowInt16Array.
 *
 * #GArrowUInt16ArrayBuilder is the class to create a new
 * #GArrowUInt16Array.
 *
 * #GArrowInt32ArrayBuilder is the class to create a new
 * #GArrowInt32Array.
 *
 * #GArrowUInt32ArrayBuilder is the class to create a new
 * #GArrowUInt32Array.
 *
 * #GArrowInt64ArrayBuilder is the class to create a new
 * #GArrowInt64Array.
 *
 * #GArrowUInt64ArrayBuilder is the class to create a new
 * #GArrowUInt64Array.
 *
 * #GArrowFloatArrayBuilder is the class to creating a new
 * #GArrowFloatArray.
 *
 * #GArrowDoubleArrayBuilder is the class to create a new
 * #GArrowDoubleArray.
 *
 * #GArrowBinaryArrayBuilder is the class to create a new
 * #GArrowBinaryArray.
 *
 * #GArrowStringArrayBuilder is the class to create a new
 * #GArrowStringArray.
 *
 * #GArrowListArrayBuilder is the class to create a new
 * #GArrowListArray.
 *
 * #GArrowStructArrayBuilder is the class to create a new
 * #GArrowStructArray.
 */

typedef struct GArrowArrayBuilderPrivate_ {
  std::shared_ptr<arrow::ArrayBuilder> array_builder;
} GArrowArrayBuilderPrivate;

enum {
  PROP_0,
  PROP_ARRAY_BUILDER
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(GArrowArrayBuilder,
                                    garrow_array_builder,
                                    G_TYPE_OBJECT)

#define GARROW_ARRAY_BUILDER_GET_PRIVATE(obj)                           \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                   \
                               GARROW_TYPE_ARRAY_BUILDER,               \
                               GArrowArrayBuilderPrivate))

static void
garrow_array_builder_finalize(GObject *object)
{
  GArrowArrayBuilderPrivate *priv;

  priv = GARROW_ARRAY_BUILDER_GET_PRIVATE(object);

  priv->array_builder = nullptr;

  G_OBJECT_CLASS(garrow_array_builder_parent_class)->finalize(object);
}

static void
garrow_array_builder_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  GArrowArrayBuilderPrivate *priv;

  priv = GARROW_ARRAY_BUILDER_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_ARRAY_BUILDER:
    priv->array_builder =
      *static_cast<std::shared_ptr<arrow::ArrayBuilder> *>(g_value_get_pointer(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_array_builder_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  switch (prop_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_array_builder_init(GArrowArrayBuilder *builder)
{
}

static void
garrow_array_builder_class_init(GArrowArrayBuilderClass *klass)
{
  GObjectClass *gobject_class;
  GParamSpec *spec;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize     = garrow_array_builder_finalize;
  gobject_class->set_property = garrow_array_builder_set_property;
  gobject_class->get_property = garrow_array_builder_get_property;

  spec = g_param_spec_pointer("array-builder",
                              "Array builder",
                              "The raw std::shared<arrow::ArrayBuilder> *",
                              static_cast<GParamFlags>(G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(gobject_class, PROP_ARRAY_BUILDER, spec);
}

/**
 * garrow_array_builder_finish:
 * @builder: A #GArrowArrayBuilder.
 *
 * Returns: (transfer full): The built #GArrowArray.
 */
GArrowArray *
garrow_array_builder_finish(GArrowArrayBuilder *builder)
{
  auto arrow_builder = garrow_array_builder_get_raw(builder);
  std::shared_ptr<arrow::Array> arrow_array;
  arrow_builder->Finish(&arrow_array);
  return garrow_array_new_raw(&arrow_array);
}


G_DEFINE_TYPE(GArrowBooleanArrayBuilder,
              garrow_boolean_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_boolean_array_builder_init(GArrowBooleanArrayBuilder *builder)
{
}

static void
garrow_boolean_array_builder_class_init(GArrowBooleanArrayBuilderClass *klass)
{
}

/**
 * garrow_boolean_array_builder_new:
 *
 * Returns: A newly created #GArrowBooleanArrayBuilder.
 */
GArrowBooleanArrayBuilder *
garrow_boolean_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_boolean_builder =
    std::make_shared<arrow::BooleanBuilder>(memory_pool);
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_boolean_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_BOOLEAN_ARRAY_BUILDER(builder);
}

/**
 * garrow_boolean_array_builder_append:
 * @builder: A #GArrowBooleanArrayBuilder.
 * @value: A boolean value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_boolean_array_builder_append(GArrowBooleanArrayBuilder *builder,
                                    gboolean value,
                                    GError **error)
{
  auto arrow_builder =
    static_cast<arrow::BooleanBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(static_cast<bool>(value));
  return garrow_error_check(error, status, "[boolean-array-builder][append]");
}

/**
 * garrow_boolean_array_builder_append_null:
 * @builder: A #GArrowBooleanArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_boolean_array_builder_append_null(GArrowBooleanArrayBuilder *builder,
                                         GError **error)
{
  auto arrow_builder =
    static_cast<arrow::BooleanBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error,
                            status,
                            "[boolean-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowInt8ArrayBuilder,
              garrow_int8_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_int8_array_builder_init(GArrowInt8ArrayBuilder *builder)
{
}

static void
garrow_int8_array_builder_class_init(GArrowInt8ArrayBuilderClass *klass)
{
}

/**
 * garrow_int8_array_builder_new:
 *
 * Returns: A newly created #GArrowInt8ArrayBuilder.
 */
GArrowInt8ArrayBuilder *
garrow_int8_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_int8_builder =
    std::make_shared<arrow::Int8Builder>(memory_pool, arrow::int8());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_int8_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_INT8_ARRAY_BUILDER(builder);
}

/**
 * garrow_int8_array_builder_append:
 * @builder: A #GArrowInt8ArrayBuilder.
 * @value: A int8 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int8_array_builder_append(GArrowInt8ArrayBuilder *builder,
                                 gint8 value,
                                 GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int8Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[int8-array-builder][append]");
}

/**
 * garrow_int8_array_builder_append_null:
 * @builder: A #GArrowInt8ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int8_array_builder_append_null(GArrowInt8ArrayBuilder *builder,
                                      GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int8Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[int8-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowUInt8ArrayBuilder,
              garrow_uint8_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_uint8_array_builder_init(GArrowUInt8ArrayBuilder *builder)
{
}

static void
garrow_uint8_array_builder_class_init(GArrowUInt8ArrayBuilderClass *klass)
{
}

/**
 * garrow_uint8_array_builder_new:
 *
 * Returns: A newly created #GArrowUInt8ArrayBuilder.
 */
GArrowUInt8ArrayBuilder *
garrow_uint8_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_uint8_builder =
    std::make_shared<arrow::UInt8Builder>(memory_pool, arrow::uint8());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_uint8_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_UINT8_ARRAY_BUILDER(builder);
}

/**
 * garrow_uint8_array_builder_append:
 * @builder: A #GArrowUInt8ArrayBuilder.
 * @value: An uint8 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint8_array_builder_append(GArrowUInt8ArrayBuilder *builder,
                                  guint8 value,
                                  GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt8Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[uint8-array-builder][append]");
}

/**
 * garrow_uint8_array_builder_append_null:
 * @builder: A #GArrowUInt8ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint8_array_builder_append_null(GArrowUInt8ArrayBuilder *builder,
                                       GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt8Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[uint8-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowInt16ArrayBuilder,
              garrow_int16_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_int16_array_builder_init(GArrowInt16ArrayBuilder *builder)
{
}

static void
garrow_int16_array_builder_class_init(GArrowInt16ArrayBuilderClass *klass)
{
}

/**
 * garrow_int16_array_builder_new:
 *
 * Returns: A newly created #GArrowInt16ArrayBuilder.
 */
GArrowInt16ArrayBuilder *
garrow_int16_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_int16_builder =
    std::make_shared<arrow::Int16Builder>(memory_pool, arrow::int16());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_int16_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_INT16_ARRAY_BUILDER(builder);
}

/**
 * garrow_int16_array_builder_append:
 * @builder: A #GArrowInt16ArrayBuilder.
 * @value: A int16 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int16_array_builder_append(GArrowInt16ArrayBuilder *builder,
                                  gint16 value,
                                  GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int16Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[int16-array-builder][append]");
}

/**
 * garrow_int16_array_builder_append_null:
 * @builder: A #GArrowInt16ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int16_array_builder_append_null(GArrowInt16ArrayBuilder *builder,
                                       GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int16Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[int16-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowUInt16ArrayBuilder,
              garrow_uint16_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_uint16_array_builder_init(GArrowUInt16ArrayBuilder *builder)
{
}

static void
garrow_uint16_array_builder_class_init(GArrowUInt16ArrayBuilderClass *klass)
{
}

/**
 * garrow_uint16_array_builder_new:
 *
 * Returns: A newly created #GArrowUInt16ArrayBuilder.
 */
GArrowUInt16ArrayBuilder *
garrow_uint16_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_uint16_builder =
    std::make_shared<arrow::UInt16Builder>(memory_pool, arrow::uint16());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_uint16_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_UINT16_ARRAY_BUILDER(builder);
}

/**
 * garrow_uint16_array_builder_append:
 * @builder: A #GArrowUInt16ArrayBuilder.
 * @value: An uint16 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint16_array_builder_append(GArrowUInt16ArrayBuilder *builder,
                                   guint16 value,
                                   GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt16Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[uint16-array-builder][append]");
}

/**
 * garrow_uint16_array_builder_append_null:
 * @builder: A #GArrowUInt16ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint16_array_builder_append_null(GArrowUInt16ArrayBuilder *builder,
                                        GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt16Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error,
                            status,
                            "[uint16-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowInt32ArrayBuilder,
              garrow_int32_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_int32_array_builder_init(GArrowInt32ArrayBuilder *builder)
{
}

static void
garrow_int32_array_builder_class_init(GArrowInt32ArrayBuilderClass *klass)
{
}

/**
 * garrow_int32_array_builder_new:
 *
 * Returns: A newly created #GArrowInt32ArrayBuilder.
 */
GArrowInt32ArrayBuilder *
garrow_int32_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_int32_builder =
    std::make_shared<arrow::Int32Builder>(memory_pool, arrow::int32());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_int32_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_INT32_ARRAY_BUILDER(builder);
}

/**
 * garrow_int32_array_builder_append:
 * @builder: A #GArrowInt32ArrayBuilder.
 * @value: A int32 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int32_array_builder_append(GArrowInt32ArrayBuilder *builder,
                                  gint32 value,
                                  GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int32Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[int32-array-builder][append]");
}

/**
 * garrow_int32_array_builder_append_null:
 * @builder: A #GArrowInt32ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int32_array_builder_append_null(GArrowInt32ArrayBuilder *builder,
                                      GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int32Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[int32-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowUInt32ArrayBuilder,
              garrow_uint32_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_uint32_array_builder_init(GArrowUInt32ArrayBuilder *builder)
{
}

static void
garrow_uint32_array_builder_class_init(GArrowUInt32ArrayBuilderClass *klass)
{
}

/**
 * garrow_uint32_array_builder_new:
 *
 * Returns: A newly created #GArrowUInt32ArrayBuilder.
 */
GArrowUInt32ArrayBuilder *
garrow_uint32_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_uint32_builder =
    std::make_shared<arrow::UInt32Builder>(memory_pool, arrow::uint32());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_uint32_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_UINT32_ARRAY_BUILDER(builder);
}

/**
 * garrow_uint32_array_builder_append:
 * @builder: A #GArrowUInt32ArrayBuilder.
 * @value: An uint32 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint32_array_builder_append(GArrowUInt32ArrayBuilder *builder,
                                   guint32 value,
                                   GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt32Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[uint32-array-builder][append]");
}

/**
 * garrow_uint32_array_builder_append_null:
 * @builder: A #GArrowUInt32ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint32_array_builder_append_null(GArrowUInt32ArrayBuilder *builder,
                                        GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt32Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error,
                            status,
                            "[uint32-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowInt64ArrayBuilder,
              garrow_int64_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_int64_array_builder_init(GArrowInt64ArrayBuilder *builder)
{
}

static void
garrow_int64_array_builder_class_init(GArrowInt64ArrayBuilderClass *klass)
{
}

/**
 * garrow_int64_array_builder_new:
 *
 * Returns: A newly created #GArrowInt64ArrayBuilder.
 */
GArrowInt64ArrayBuilder *
garrow_int64_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_int64_builder =
    std::make_shared<arrow::Int64Builder>(memory_pool, arrow::int64());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_int64_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_INT64_ARRAY_BUILDER(builder);
}

/**
 * garrow_int64_array_builder_append:
 * @builder: A #GArrowInt64ArrayBuilder.
 * @value: A int64 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int64_array_builder_append(GArrowInt64ArrayBuilder *builder,
                                  gint64 value,
                                  GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int64Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[int64-array-builder][append]");
}

/**
 * garrow_int64_array_builder_append_null:
 * @builder: A #GArrowInt64ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_int64_array_builder_append_null(GArrowInt64ArrayBuilder *builder,
                                       GError **error)
{
  auto arrow_builder =
    static_cast<arrow::Int64Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[int64-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowUInt64ArrayBuilder,
              garrow_uint64_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_uint64_array_builder_init(GArrowUInt64ArrayBuilder *builder)
{
}

static void
garrow_uint64_array_builder_class_init(GArrowUInt64ArrayBuilderClass *klass)
{
}

/**
 * garrow_uint64_array_builder_new:
 *
 * Returns: A newly created #GArrowUInt64ArrayBuilder.
 */
GArrowUInt64ArrayBuilder *
garrow_uint64_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_uint64_builder =
    std::make_shared<arrow::UInt64Builder>(memory_pool, arrow::uint64());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_uint64_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_UINT64_ARRAY_BUILDER(builder);
}

/**
 * garrow_uint64_array_builder_append:
 * @builder: A #GArrowUInt64ArrayBuilder.
 * @value: An uint64 value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint64_array_builder_append(GArrowUInt64ArrayBuilder *builder,
                                  guint64 value,
                                  GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt64Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[uint64-array-builder][append]");
}

/**
 * garrow_uint64_array_builder_append_null:
 * @builder: A #GArrowUInt64ArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_uint64_array_builder_append_null(GArrowUInt64ArrayBuilder *builder,
                                       GError **error)
{
  auto arrow_builder =
    static_cast<arrow::UInt64Builder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  if (status.ok()) {
    return TRUE;
  } else {
    garrow_error_check(error, status, "[uint64-array-builder][append-null]");
    return FALSE;
  }
}


G_DEFINE_TYPE(GArrowFloatArrayBuilder,
              garrow_float_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_float_array_builder_init(GArrowFloatArrayBuilder *builder)
{
}

static void
garrow_float_array_builder_class_init(GArrowFloatArrayBuilderClass *klass)
{
}

/**
 * garrow_float_array_builder_new:
 *
 * Returns: A newly created #GArrowFloatArrayBuilder.
 */
GArrowFloatArrayBuilder *
garrow_float_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_float_builder =
    std::make_shared<arrow::FloatBuilder>(memory_pool, arrow::float32());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_float_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_FLOAT_ARRAY_BUILDER(builder);
}

/**
 * garrow_float_array_builder_append:
 * @builder: A #GArrowFloatArrayBuilder.
 * @value: A float value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_float_array_builder_append(GArrowFloatArrayBuilder *builder,
                                  gfloat value,
                                  GError **error)
{
  auto arrow_builder =
    static_cast<arrow::FloatBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[float-array-builder][append]");
}

/**
 * garrow_float_array_builder_append_null:
 * @builder: A #GArrowFloatArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_float_array_builder_append_null(GArrowFloatArrayBuilder *builder,
                                       GError **error)
{
  auto arrow_builder =
    static_cast<arrow::FloatBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[float-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowDoubleArrayBuilder,
              garrow_double_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_double_array_builder_init(GArrowDoubleArrayBuilder *builder)
{
}

static void
garrow_double_array_builder_class_init(GArrowDoubleArrayBuilderClass *klass)
{
}

/**
 * garrow_double_array_builder_new:
 *
 * Returns: A newly created #GArrowDoubleArrayBuilder.
 */
GArrowDoubleArrayBuilder *
garrow_double_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_double_builder =
    std::make_shared<arrow::DoubleBuilder>(memory_pool, arrow::float64());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_double_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_DOUBLE_ARRAY_BUILDER(builder);
}

/**
 * garrow_double_array_builder_append:
 * @builder: A #GArrowDoubleArrayBuilder.
 * @value: A double value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_double_array_builder_append(GArrowDoubleArrayBuilder *builder,
                                   gdouble value,
                                   GError **error)
{
  auto arrow_builder =
    static_cast<arrow::DoubleBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value);
  return garrow_error_check(error, status, "[double-array-builder][append]");
}

/**
 * garrow_double_array_builder_append_null:
 * @builder: A #GArrowDoubleArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_double_array_builder_append_null(GArrowDoubleArrayBuilder *builder,
                                        GError **error)
{
  auto arrow_builder =
    static_cast<arrow::DoubleBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error,
                            status,
                            "[double-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowBinaryArrayBuilder,
              garrow_binary_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_binary_array_builder_init(GArrowBinaryArrayBuilder *builder)
{
}

static void
garrow_binary_array_builder_class_init(GArrowBinaryArrayBuilderClass *klass)
{
}

/**
 * garrow_binary_array_builder_new:
 *
 * Returns: A newly created #GArrowBinaryArrayBuilder.
 */
GArrowBinaryArrayBuilder *
garrow_binary_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_binary_builder =
    std::make_shared<arrow::BinaryBuilder>(memory_pool, arrow::binary());
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_binary_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_BINARY_ARRAY_BUILDER(builder);
}

/**
 * garrow_binary_array_builder_append:
 * @builder: A #GArrowBinaryArrayBuilder.
 * @value: (array length=length): A binary value.
 * @length: A value length.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_binary_array_builder_append(GArrowBinaryArrayBuilder *builder,
                                   const guint8 *value,
                                   gint32 length,
                                   GError **error)
{
  auto arrow_builder =
    static_cast<arrow::BinaryBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value, length);
  return garrow_error_check(error, status, "[binary-array-builder][append]");
}

/**
 * garrow_binary_array_builder_append_null:
 * @builder: A #GArrowBinaryArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_binary_array_builder_append_null(GArrowBinaryArrayBuilder *builder,
                                        GError **error)
{
  auto arrow_builder =
    static_cast<arrow::BinaryBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error,
                            status,
                            "[binary-array-builder][append-null]");
}


G_DEFINE_TYPE(GArrowStringArrayBuilder,
              garrow_string_array_builder,
              GARROW_TYPE_BINARY_ARRAY_BUILDER)

static void
garrow_string_array_builder_init(GArrowStringArrayBuilder *builder)
{
}

static void
garrow_string_array_builder_class_init(GArrowStringArrayBuilderClass *klass)
{
}

/**
 * garrow_string_array_builder_new:
 *
 * Returns: A newly created #GArrowStringArrayBuilder.
 */
GArrowStringArrayBuilder *
garrow_string_array_builder_new(void)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_string_builder =
    std::make_shared<arrow::StringBuilder>(memory_pool);
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_string_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_STRING_ARRAY_BUILDER(builder);
}

/**
 * garrow_string_array_builder_append:
 * @builder: A #GArrowStringArrayBuilder.
 * @value: A string value.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_string_array_builder_append(GArrowStringArrayBuilder *builder,
                                   const gchar *value,
                                   GError **error)
{
  auto arrow_builder =
    static_cast<arrow::StringBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append(value,
                                      static_cast<gint32>(strlen(value)));
  return garrow_error_check(error, status, "[string-array-builder][append]");
}


G_DEFINE_TYPE(GArrowListArrayBuilder,
              garrow_list_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_list_array_builder_init(GArrowListArrayBuilder *builder)
{
}

static void
garrow_list_array_builder_class_init(GArrowListArrayBuilderClass *klass)
{
}

/**
 * garrow_list_array_builder_new:
 * @value_builder: A #GArrowArrayBuilder for value array.
 *
 * Returns: A newly created #GArrowListArrayBuilder.
 */
GArrowListArrayBuilder *
garrow_list_array_builder_new(GArrowArrayBuilder *value_builder)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_value_builder = garrow_array_builder_get_raw(value_builder);
  auto arrow_list_builder =
    std::make_shared<arrow::ListBuilder>(memory_pool, arrow_value_builder);
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_list_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_LIST_ARRAY_BUILDER(builder);
}

/**
 * garrow_list_array_builder_append:
 * @builder: A #GArrowListArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 *
 * It appends a new list element. To append a new list element, you
 * need to call this function then append list element values to
 * `value_builder`. `value_builder` is the #GArrowArrayBuilder
 * specified to constructor. You can get `value_builder` by
 * garrow_list_array_builder_get_value_builder().
 *
 * |[<!-- language="C" -->
 * GArrowInt8ArrayBuilder *value_builder;
 * GArrowListArrayBuilder *builder;
 *
 * value_builder = garrow_int8_array_builder_new();
 * builder = garrow_list_array_builder_new(value_builder, NULL);
 *
 * // Start 0th list element: [1, 0, -1]
 * garrow_list_array_builder_append(builder, NULL);
 * garrow_int8_array_builder_append(value_builder, 1);
 * garrow_int8_array_builder_append(value_builder, 0);
 * garrow_int8_array_builder_append(value_builder, -1);
 *
 * // Start 1st list element: [-29, 29]
 * garrow_list_array_builder_append(builder, NULL);
 * garrow_int8_array_builder_append(value_builder, -29);
 * garrow_int8_array_builder_append(value_builder, 29);
 *
 * {
 *   // [[1, 0, -1], [-29, 29]]
 *   GArrowArray *array = garrow_array_builder_finish(builder);
 *   // Now, builder is needless.
 *   g_object_unref(builder);
 *   g_object_unref(value_builder);
 *
 *   // Use array...
 *   g_object_unref(array);
 * }
 * ]|
 */
gboolean
garrow_list_array_builder_append(GArrowListArrayBuilder *builder,
                                 GError **error)
{
  auto arrow_builder =
    static_cast<arrow::ListBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append();
  return garrow_error_check(error, status, "[list-array-builder][append]");
}

/**
 * garrow_list_array_builder_append_null:
 * @builder: A #GArrowListArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 *
 * It appends a new NULL element.
 */
gboolean
garrow_list_array_builder_append_null(GArrowListArrayBuilder *builder,
                                      GError **error)
{
  auto arrow_builder =
    static_cast<arrow::ListBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error, status, "[list-array-builder][append-null]");
}

/**
 * garrow_list_array_builder_get_value_builder:
 * @builder: A #GArrowListArrayBuilder.
 *
 * Returns: (transfer full): The #GArrowArrayBuilder for values.
 */
GArrowArrayBuilder *
garrow_list_array_builder_get_value_builder(GArrowListArrayBuilder *builder)
{
  auto arrow_builder =
    static_cast<arrow::ListBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());
  auto arrow_value_builder = arrow_builder->value_builder();
  return garrow_array_builder_new_raw(&arrow_value_builder);
}


G_DEFINE_TYPE(GArrowStructArrayBuilder,
              garrow_struct_array_builder,
              GARROW_TYPE_ARRAY_BUILDER)

static void
garrow_struct_array_builder_init(GArrowStructArrayBuilder *builder)
{
}

static void
garrow_struct_array_builder_class_init(GArrowStructArrayBuilderClass *klass)
{
}

/**
 * garrow_struct_array_builder_new:
 * @data_type: #GArrowStructDataType for the struct.
 * @field_builders: (element-type GArrowArray): #GArrowArrayBuilders
 *   for fields.
 *
 * Returns: A newly created #GArrowStructArrayBuilder.
 */
GArrowStructArrayBuilder *
garrow_struct_array_builder_new(GArrowStructDataType *data_type,
                                GList *field_builders)
{
  auto memory_pool = arrow::default_memory_pool();
  auto arrow_data_type = garrow_data_type_get_raw(GARROW_DATA_TYPE(data_type));
  std::vector<std::shared_ptr<arrow::ArrayBuilder>> arrow_field_builders;
  for (GList *node = field_builders; node; node = g_list_next(node)) {
    auto field_builder = static_cast<GArrowArrayBuilder *>(node->data);
    auto arrow_field_builder = garrow_array_builder_get_raw(field_builder);
    arrow_field_builders.push_back(arrow_field_builder);
  }

  auto arrow_struct_builder =
    std::make_shared<arrow::StructBuilder>(memory_pool,
                                           arrow_data_type,
                                           arrow_field_builders);
  std::shared_ptr<arrow::ArrayBuilder> arrow_builder = arrow_struct_builder;
  auto builder = garrow_array_builder_new_raw(&arrow_builder);
  return GARROW_STRUCT_ARRAY_BUILDER(builder);
}

/**
 * garrow_struct_array_builder_append:
 * @builder: A #GArrowStructArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 *
 * It appends a new struct element. To append a new struct element,
 * you need to call this function then append struct element field
 * values to all `field_builder`s. `field_value`s are the
 * #GArrowArrayBuilder specified to constructor. You can get
 * `field_builder` by garrow_struct_array_builder_get_field_builder()
 * or garrow_struct_array_builder_get_field_builders().
 *
 * |[<!-- language="C" -->
 * // TODO
 * ]|
 */
gboolean
garrow_struct_array_builder_append(GArrowStructArrayBuilder *builder,
                                   GError **error)
{
  auto arrow_builder =
    static_cast<arrow::StructBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->Append();
  return garrow_error_check(error, status, "[struct-array-builder][append]");
}

/**
 * garrow_struct_array_builder_append_null:
 * @builder: A #GArrowStructArrayBuilder.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 *
 * It appends a new NULL element.
 */
gboolean
garrow_struct_array_builder_append_null(GArrowStructArrayBuilder *builder,
                                        GError **error)
{
  auto arrow_builder =
    static_cast<arrow::StructBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  auto status = arrow_builder->AppendNull();
  return garrow_error_check(error,
                            status,
                            "[struct-array-builder][append-null]");
}

/**
 * garrow_struct_array_builder_get_field_builder:
 * @builder: A #GArrowStructArrayBuilder.
 * @i: The index of the field in the struct.
 *
 * Returns: (transfer full): The #GArrowArrayBuilder for the i-th field.
 */
GArrowArrayBuilder *
garrow_struct_array_builder_get_field_builder(GArrowStructArrayBuilder *builder,
                                              gint i)
{
  auto arrow_builder =
    static_cast<arrow::StructBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());
  auto arrow_field_builder = arrow_builder->field_builder(i);
  return garrow_array_builder_new_raw(&arrow_field_builder);
}

/**
 * garrow_struct_array_builder_get_field_builders:
 * @builder: A #GArrowStructArrayBuilder.
 *
 * Returns: (element-type GArrowArray) (transfer full):
 *   The #GArrowArrayBuilder for all fields.
 */
GList *
garrow_struct_array_builder_get_field_builders(GArrowStructArrayBuilder *builder)
{
  auto arrow_struct_builder =
    static_cast<arrow::StructBuilder *>(
      garrow_array_builder_get_raw(GARROW_ARRAY_BUILDER(builder)).get());

  GList *field_builders = NULL;
  for (auto arrow_field_builder : arrow_struct_builder->field_builders()) {
    auto field_builder = garrow_array_builder_new_raw(&arrow_field_builder);
    field_builders = g_list_prepend(field_builders, field_builder);
  }

  return g_list_reverse(field_builders);
}


G_END_DECLS

GArrowArrayBuilder *
garrow_array_builder_new_raw(std::shared_ptr<arrow::ArrayBuilder> *arrow_builder)
{
  GType type;

  switch ((*arrow_builder)->type()->id()) {
  case arrow::Type::type::BOOL:
    type = GARROW_TYPE_BOOLEAN_ARRAY_BUILDER;
    break;
  case arrow::Type::type::UINT8:
    type = GARROW_TYPE_UINT8_ARRAY_BUILDER;
    break;
  case arrow::Type::type::INT8:
    type = GARROW_TYPE_INT8_ARRAY_BUILDER;
    break;
  case arrow::Type::type::UINT16:
    type = GARROW_TYPE_UINT16_ARRAY_BUILDER;
    break;
  case arrow::Type::type::INT16:
    type = GARROW_TYPE_INT16_ARRAY_BUILDER;
    break;
  case arrow::Type::type::UINT32:
    type = GARROW_TYPE_UINT32_ARRAY_BUILDER;
    break;
  case arrow::Type::type::INT32:
    type = GARROW_TYPE_INT32_ARRAY_BUILDER;
    break;
  case arrow::Type::type::UINT64:
    type = GARROW_TYPE_UINT64_ARRAY_BUILDER;
    break;
  case arrow::Type::type::INT64:
    type = GARROW_TYPE_INT64_ARRAY_BUILDER;
    break;
  case arrow::Type::type::FLOAT:
    type = GARROW_TYPE_FLOAT_ARRAY_BUILDER;
    break;
  case arrow::Type::type::DOUBLE:
    type = GARROW_TYPE_DOUBLE_ARRAY_BUILDER;
    break;
  case arrow::Type::type::BINARY:
    type = GARROW_TYPE_BINARY_ARRAY_BUILDER;
    break;
  case arrow::Type::type::STRING:
    type = GARROW_TYPE_STRING_ARRAY_BUILDER;
    break;
  case arrow::Type::type::LIST:
    type = GARROW_TYPE_LIST_ARRAY_BUILDER;
    break;
  case arrow::Type::type::STRUCT:
    type = GARROW_TYPE_STRUCT_ARRAY_BUILDER;
    break;
  default:
    type = GARROW_TYPE_ARRAY_BUILDER;
    break;
  }

  auto builder =
    GARROW_ARRAY_BUILDER(g_object_new(type,
                                      "array-builder", arrow_builder,
                                      NULL));
  return builder;
}

std::shared_ptr<arrow::ArrayBuilder>
garrow_array_builder_get_raw(GArrowArrayBuilder *builder)
{
  GArrowArrayBuilderPrivate *priv;

  priv = GARROW_ARRAY_BUILDER_GET_PRIVATE(builder);
  return priv->array_builder;
}
