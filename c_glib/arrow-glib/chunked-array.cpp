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
#include <arrow-glib/chunked-array.hpp>

G_BEGIN_DECLS

/**
 * SECTION: chunked-array
 * @short_description: Chunked array class
 *
 * #GArrowChunkedArray is a class for chunked array. Chunked array
 * makes a list of #GArrowArrays one logical large array.
 */

typedef struct GArrowChunkedArrayPrivate_ {
  std::shared_ptr<arrow::ChunkedArray> chunked_array;
} GArrowChunkedArrayPrivate;

enum {
  PROP_0,
  PROP_CHUNKED_ARRAY
};

G_DEFINE_TYPE_WITH_PRIVATE(GArrowChunkedArray,
                           garrow_chunked_array,
                           G_TYPE_OBJECT)

#define GARROW_CHUNKED_ARRAY_GET_PRIVATE(obj)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                               GARROW_TYPE_CHUNKED_ARRAY,       \
                               GArrowChunkedArrayPrivate))

static void
garrow_chunked_array_finalize(GObject *object)
{
  GArrowChunkedArrayPrivate *priv;

  priv = GARROW_CHUNKED_ARRAY_GET_PRIVATE(object);

  priv->chunked_array = nullptr;

  G_OBJECT_CLASS(garrow_chunked_array_parent_class)->finalize(object);
}

static void
garrow_chunked_array_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  GArrowChunkedArrayPrivate *priv;

  priv = GARROW_CHUNKED_ARRAY_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_CHUNKED_ARRAY:
    priv->chunked_array =
      *static_cast<std::shared_ptr<arrow::ChunkedArray> *>(g_value_get_pointer(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_chunked_array_get_property(GObject *object,
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
garrow_chunked_array_init(GArrowChunkedArray *object)
{
}

static void
garrow_chunked_array_class_init(GArrowChunkedArrayClass *klass)
{
  GObjectClass *gobject_class;
  GParamSpec *spec;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize     = garrow_chunked_array_finalize;
  gobject_class->set_property = garrow_chunked_array_set_property;
  gobject_class->get_property = garrow_chunked_array_get_property;

  spec = g_param_spec_pointer("chunked-array",
                              "Chunked array",
                              "The raw std::shared<arrow::ChunkedArray> *",
                              static_cast<GParamFlags>(G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(gobject_class, PROP_CHUNKED_ARRAY, spec);
}

/**
 * garrow_chunked_array_new:
 * @chunks: (element-type GArrowArray): The array chunks.
 *
 * Returns: A newly created #GArrowChunkedArray.
 */
GArrowChunkedArray *
garrow_chunked_array_new(GList *chunks)
{
  std::vector<std::shared_ptr<arrow::Array>> arrow_chunks;
  for (GList *node = chunks; node; node = node->next) {
    GArrowArray *chunk = GARROW_ARRAY(node->data);
    arrow_chunks.push_back(garrow_array_get_raw(chunk));
  }

  auto arrow_chunked_array =
    std::make_shared<arrow::ChunkedArray>(arrow_chunks);
  return garrow_chunked_array_new_raw(&arrow_chunked_array);
}

/**
 * garrow_chunked_array_get_length:
 * @chunked_array: A #GArrowChunkedArray.
 *
 * Returns: The total number of rows in the chunked array.
 */
guint64
garrow_chunked_array_get_length(GArrowChunkedArray *chunked_array)
{
  const auto arrow_chunked_array = garrow_chunked_array_get_raw(chunked_array);
  return arrow_chunked_array->length();
}

/**
 * garrow_chunked_array_get_n_nulls:
 * @chunked_array: A #GArrowChunkedArray.
 *
 * Returns: The total number of NULL in the chunked array.
 */
guint64
garrow_chunked_array_get_n_nulls(GArrowChunkedArray *chunked_array)
{
  const auto arrow_chunked_array = garrow_chunked_array_get_raw(chunked_array);
  return arrow_chunked_array->null_count();
}

/**
 * garrow_chunked_array_get_n_chunks:
 * @chunked_array: A #GArrowChunkedArray.
 *
 * Returns: The total number of chunks in the chunked array.
 */
guint
garrow_chunked_array_get_n_chunks(GArrowChunkedArray *chunked_array)
{
  const auto arrow_chunked_array = garrow_chunked_array_get_raw(chunked_array);
  return arrow_chunked_array->num_chunks();
}

/**
 * garrow_chunked_array_get_chunk:
 * @chunked_array: A #GArrowChunkedArray.
 * @i: The index of the target chunk.
 *
 * Returns: (transfer full): The i-th chunk of the chunked array.
 */
GArrowArray *
garrow_chunked_array_get_chunk(GArrowChunkedArray *chunked_array,
                               guint i)
{
  const auto arrow_chunked_array = garrow_chunked_array_get_raw(chunked_array);
  auto arrow_chunk = arrow_chunked_array->chunk(i);
  return garrow_array_new_raw(&arrow_chunk);
}

/**
 * garrow_chunked_array_get_chunks:
 * @chunked_array: A #GArrowChunkedArray.
 *
 * Returns: (element-type GArrowArray) (transfer full):
 *   The chunks in the chunked array.
 */
GList *
garrow_chunked_array_get_chunks(GArrowChunkedArray *chunked_array)
{
  const auto arrow_chunked_array = garrow_chunked_array_get_raw(chunked_array);

  GList *chunks = NULL;
  for (auto arrow_chunk : arrow_chunked_array->chunks()) {
    GArrowArray *chunk = garrow_array_new_raw(&arrow_chunk);
    chunks = g_list_prepend(chunks, chunk);
  }

  return g_list_reverse(chunks);
}

G_END_DECLS

GArrowChunkedArray *
garrow_chunked_array_new_raw(std::shared_ptr<arrow::ChunkedArray> *arrow_chunked_array)
{
  auto chunked_array =
    GARROW_CHUNKED_ARRAY(g_object_new(GARROW_TYPE_CHUNKED_ARRAY,
                                      "chunked-array", arrow_chunked_array,
                                      NULL));
  return chunked_array;
}

std::shared_ptr<arrow::ChunkedArray>
garrow_chunked_array_get_raw(GArrowChunkedArray *chunked_array)
{
  GArrowChunkedArrayPrivate *priv;

  priv = GARROW_CHUNKED_ARRAY_GET_PRIVATE(chunked_array);
  return priv->chunked_array;
}
