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

#include <arrow-glib/compute.hpp>
#include <arrow-glib/enums.h>

G_BEGIN_DECLS

/**
 * SECTION: compute
 * @section_id: compute-classes
 * @title: Classes for computation
 * @include: arrow-glib/arrow-glib.h
 *
 * #GArrowCastOptions is a class to customize garrow_array_cast().
 *
 * #GArrowCountOptions is a class to customize garrow_array_count().
 */

typedef struct GArrowCastOptionsPrivate_ {
  arrow::compute::CastOptions options;
} GArrowCastOptionsPrivate;

enum {
  PROP_ALLOW_INT_OVERFLOW = 1,
  PROP_ALLOW_TIME_TRUNCATE,
  PROP_ALLOW_FLOAT_TRUNCATE,
  PROP_ALLOW_INVALID_UTF8,
};

G_DEFINE_TYPE_WITH_PRIVATE(GArrowCastOptions,
                           garrow_cast_options,
                           G_TYPE_OBJECT)

#define GARROW_CAST_OPTIONS_GET_PRIVATE(object) \
  static_cast<GArrowCastOptionsPrivate *>(      \
    garrow_cast_options_get_instance_private(   \
      GARROW_CAST_OPTIONS(object)))

static void
garrow_cast_options_set_property(GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  auto priv = GARROW_CAST_OPTIONS_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_ALLOW_INT_OVERFLOW:
    priv->options.allow_int_overflow = g_value_get_boolean(value);
    break;
  case PROP_ALLOW_TIME_TRUNCATE:
    priv->options.allow_time_truncate = g_value_get_boolean(value);
    break;
  case PROP_ALLOW_FLOAT_TRUNCATE:
    priv->options.allow_float_truncate = g_value_get_boolean(value);
    break;
  case PROP_ALLOW_INVALID_UTF8:
    priv->options.allow_invalid_utf8 = g_value_get_boolean(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_cast_options_get_property(GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  auto priv = GARROW_CAST_OPTIONS_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_ALLOW_INT_OVERFLOW:
    g_value_set_boolean(value, priv->options.allow_int_overflow);
    break;
  case PROP_ALLOW_TIME_TRUNCATE:
    g_value_set_boolean(value, priv->options.allow_time_truncate);
    break;
  case PROP_ALLOW_FLOAT_TRUNCATE:
    g_value_set_boolean(value, priv->options.allow_float_truncate);
    break;
  case PROP_ALLOW_INVALID_UTF8:
    g_value_set_boolean(value, priv->options.allow_invalid_utf8);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_cast_options_init(GArrowCastOptions *object)
{
}

static void
garrow_cast_options_class_init(GArrowCastOptionsClass *klass)
{
  auto gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = garrow_cast_options_set_property;
  gobject_class->get_property = garrow_cast_options_get_property;

  GParamSpec *spec;
  /**
   * GArrowCastOptions:allow-int-overflow:
   *
   * Whether integer overflow is allowed or not.
   *
   * Since: 0.7.0
   */
  spec = g_param_spec_boolean("allow-int-overflow",
                              "Allow int overflow",
                              "Whether integer overflow is allowed or not",
                              FALSE,
                              static_cast<GParamFlags>(G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ALLOW_INT_OVERFLOW, spec);

  /**
   * GArrowCastOptions:allow-time-truncate:
   *
   * Whether truncating time value is allowed or not.
   *
   * Since: 0.8.0
   */
  spec = g_param_spec_boolean("allow-time-truncate",
                              "Allow time truncate",
                              "Whether truncating time value is allowed or not",
                              FALSE,
                              static_cast<GParamFlags>(G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ALLOW_TIME_TRUNCATE, spec);

  /**
   * GArrowCastOptions:allow-float-truncate:
   *
   * Whether truncating float value is allowed or not.
   *
   * Since: 0.12.0
   */
  spec = g_param_spec_boolean("allow-float-truncate",
                              "Allow float truncate",
                              "Whether truncating float value is allowed or not",
                              FALSE,
                              static_cast<GParamFlags>(G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ALLOW_FLOAT_TRUNCATE, spec);

  /**
   * GArrowCastOptions:allow-invalid-utf8:
   *
   * Whether invalid UTF-8 string value is allowed or not.
   *
   * Since: 0.13.0
   */
  spec = g_param_spec_boolean("allow-invalid-utf8",
                              "Allow invalid UTF-8",
                              "Whether invalid UTF-8 string value is allowed or not",
                              FALSE,
                              static_cast<GParamFlags>(G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ALLOW_INVALID_UTF8, spec);
}

/**
 * garrow_cast_options_new:
 *
 * Returns: A newly created #GArrowCastOptions.
 *
 * Since: 0.7.0
 */
GArrowCastOptions *
garrow_cast_options_new(void)
{
  auto cast_options = g_object_new(GARROW_TYPE_CAST_OPTIONS, NULL);
  return GARROW_CAST_OPTIONS(cast_options);
}


typedef struct GArrowCountOptionsPrivate_ {
  arrow::compute::CountOptions options;
} GArrowCountOptionsPrivate;

enum {
  PROP_MODE = 1,
};

G_DEFINE_TYPE_WITH_PRIVATE(GArrowCountOptions,
                           garrow_count_options,
                           G_TYPE_OBJECT)

#define GARROW_COUNT_OPTIONS_GET_PRIVATE(object)        \
  static_cast<GArrowCountOptionsPrivate *>(             \
    garrow_count_options_get_instance_private(          \
      GARROW_COUNT_OPTIONS(object)))

static void
garrow_count_options_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  auto priv = GARROW_COUNT_OPTIONS_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_MODE:
    priv->options.count_mode =
      static_cast<arrow::compute::CountOptions::mode>(g_value_get_enum(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_count_options_get_property(GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  auto priv = GARROW_COUNT_OPTIONS_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_MODE:
    g_value_set_enum(value, priv->options.count_mode);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_count_options_init(GArrowCountOptions *object)
{
}

static void
garrow_count_options_class_init(GArrowCountOptionsClass *klass)
{
  auto gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = garrow_count_options_set_property;
  gobject_class->get_property = garrow_count_options_get_property;

  GParamSpec *spec;
  /**
   * GArrowCountOptions:mode:
   *
   * How to count values.
   *
   * Since: 0.13.0
   */
  spec = g_param_spec_enum("mode",
                           "Mode",
                           "How to count values",
                           GARROW_TYPE_COUNT_MODE,
                           GARROW_COUNT_ALL,
                           static_cast<GParamFlags>(G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_MODE, spec);
}

/**
 * garrow_count_options_new:
 *
 * Returns: A newly created #GArrowCountOptions.
 *
 * Since: 0.13.0
 */
GArrowCountOptions *
garrow_count_options_new(void)
{
  auto count_options = g_object_new(GARROW_TYPE_COUNT_OPTIONS, NULL);
  return GARROW_COUNT_OPTIONS(count_options);
}

G_END_DECLS

GArrowCastOptions *
garrow_cast_options_new_raw(arrow::compute::CastOptions *arrow_cast_options)
{
  auto cast_options =
    g_object_new(GARROW_TYPE_CAST_OPTIONS,
                 "allow-int-overflow", arrow_cast_options->allow_int_overflow,
                 "allow-time-truncate", arrow_cast_options->allow_time_truncate,
                 "allow-float-truncate", arrow_cast_options->allow_float_truncate,
                 "allow-invalid-utf8", arrow_cast_options->allow_invalid_utf8,
                 NULL);
  return GARROW_CAST_OPTIONS(cast_options);
}

arrow::compute::CastOptions *
garrow_cast_options_get_raw(GArrowCastOptions *cast_options)
{
  auto priv = GARROW_CAST_OPTIONS_GET_PRIVATE(cast_options);
  return &(priv->options);
}

GArrowCountOptions *
garrow_count_options_new_raw(arrow::compute::CountOptions *arrow_count_options)
{
  auto count_options =
    g_object_new(GARROW_TYPE_COUNT_OPTIONS,
                 "mode", arrow_count_options->count_mode,
                 NULL);
  return GARROW_COUNT_OPTIONS(count_options);
}

arrow::compute::CountOptions *
garrow_count_options_get_raw(GArrowCountOptions *count_options)
{
  auto priv = GARROW_COUNT_OPTIONS_GET_PRIVATE(count_options);
  return &(priv->options);
}
