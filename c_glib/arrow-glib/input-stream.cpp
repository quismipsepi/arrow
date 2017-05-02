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

#include <arrow/io/interfaces.h>
#include <arrow/io/memory.h>

#include <arrow-glib/buffer.hpp>
#include <arrow-glib/error.hpp>
#include <arrow-glib/file.hpp>
#include <arrow-glib/input-stream.hpp>
#include <arrow-glib/readable.hpp>

G_BEGIN_DECLS

/**
 * SECTION: input-stream
 * @section_id: input-stream-classes
 * @title: Input stream classes
 * @include: arrow-glib/arrow-glib.h
 *
 * #GArrowInputStream is a base class for input stream.
 *
 * #GArrowSeekableInputStream is a base class for input stream that
 * supports random access.
 *
 * #GArrowBufferInputStream is a class to read data on buffer.
 *
 * #GArrowMemoryMappedFile is a class to read data in file by mapping
 * the file on memory. It supports zero copy.
 */

typedef struct GArrowInputStreamPrivate_ {
  std::shared_ptr<arrow::io::InputStream> input_stream;
} GArrowInputStreamPrivate;

enum {
  PROP_0,
  PROP_INPUT_STREAM
};

static std::shared_ptr<arrow::io::FileInterface>
garrow_input_stream_get_raw_file_interface(GArrowFile *file)
{
  auto input_stream = GARROW_INPUT_STREAM(file);
  auto arrow_input_stream =
    garrow_input_stream_get_raw(input_stream);
  return arrow_input_stream;
}

static void
garrow_input_stream_file_interface_init(GArrowFileInterface *iface)
{
  iface->get_raw = garrow_input_stream_get_raw_file_interface;
}

static std::shared_ptr<arrow::io::Readable>
garrow_input_stream_get_raw_readable_interface(GArrowReadable *readable)
{
  auto input_stream = GARROW_INPUT_STREAM(readable);
  auto arrow_input_stream = garrow_input_stream_get_raw(input_stream);
  return arrow_input_stream;
}

static void
garrow_input_stream_readable_interface_init(GArrowReadableInterface *iface)
{
  iface->get_raw = garrow_input_stream_get_raw_readable_interface;
}

G_DEFINE_TYPE_WITH_CODE(GArrowInputStream,
                        garrow_input_stream,
                        G_TYPE_OBJECT,
                        G_ADD_PRIVATE(GArrowInputStream)
                        G_IMPLEMENT_INTERFACE(GARROW_TYPE_FILE,
                                              garrow_input_stream_file_interface_init)
                        G_IMPLEMENT_INTERFACE(GARROW_TYPE_READABLE,
                                              garrow_input_stream_readable_interface_init));

#define GARROW_INPUT_STREAM_GET_PRIVATE(obj)                    \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                               GARROW_TYPE_INPUT_STREAM,        \
                               GArrowInputStreamPrivate))

static void
garrow_input_stream_finalize(GObject *object)
{
  GArrowInputStreamPrivate *priv;

  priv = GARROW_INPUT_STREAM_GET_PRIVATE(object);

  priv->input_stream = nullptr;

  G_OBJECT_CLASS(garrow_input_stream_parent_class)->finalize(object);
}

static void
garrow_input_stream_set_property(GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  GArrowInputStreamPrivate *priv;

  priv = GARROW_INPUT_STREAM_GET_PRIVATE(object);

  switch (prop_id) {
  case PROP_INPUT_STREAM:
    priv->input_stream =
      *static_cast<std::shared_ptr<arrow::io::InputStream> *>(g_value_get_pointer(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
garrow_input_stream_get_property(GObject *object,
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
garrow_input_stream_init(GArrowInputStream *object)
{
}

static void
garrow_input_stream_class_init(GArrowInputStreamClass *klass)
{
  GObjectClass *gobject_class;
  GParamSpec *spec;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize     = garrow_input_stream_finalize;
  gobject_class->set_property = garrow_input_stream_set_property;
  gobject_class->get_property = garrow_input_stream_get_property;

  spec = g_param_spec_pointer("input-stream",
                              "Input stream",
                              "The raw std::shared<arrow::io::InputStream> *",
                              static_cast<GParamFlags>(G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(gobject_class, PROP_INPUT_STREAM, spec);
}


G_DEFINE_TYPE(GArrowSeekableInputStream,                \
              garrow_seekable_input_stream,             \
              GARROW_TYPE_INPUT_STREAM);

static void
garrow_seekable_input_stream_init(GArrowSeekableInputStream *object)
{
}

static void
garrow_seekable_input_stream_class_init(GArrowSeekableInputStreamClass *klass)
{
}

/**
 * garrow_seekable_input_stream_get_size:
 * @input_stream: A #GArrowSeekableInputStream.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: The size of the file.
 */
guint64
garrow_seekable_input_stream_get_size(GArrowSeekableInputStream *input_stream,
                                      GError **error)
{
  auto arrow_random_access_file =
    garrow_seekable_input_stream_get_raw(input_stream);
  int64_t size;
  auto status = arrow_random_access_file->GetSize(&size);
  if (garrow_error_check(error, status, "[seekable-input-stream][get-size]")) {
    return size;
  } else {
    return 0;
  }
}

/**
 * garrow_seekable_input_stream_get_support_zero_copy:
 * @input_stream: A #GArrowSeekableInputStream.
 *
 * Returns: Whether zero copy read is supported or not.
 */
gboolean
garrow_seekable_input_stream_get_support_zero_copy(GArrowSeekableInputStream *input_stream)
{
  auto arrow_random_access_file =
    garrow_seekable_input_stream_get_raw(input_stream);
  return arrow_random_access_file->supports_zero_copy();
}

/**
 * garrow_seekable_input_stream_read_at:
 * @input_stream: A #GArrowSeekableInputStream.
 * @position: The read start position.
 * @n_bytes: The number of bytes to be read.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: (transfer full) (nullable): #GArrowBuffer that has read
 *   data on success, %NULL if there was an error.
 */
GArrowBuffer *
garrow_seekable_input_stream_read_at(GArrowSeekableInputStream *input_stream,
                                     gint64 position,
                                     gint64 n_bytes,
                                     GError **error)
{
  auto arrow_random_access_file =
    garrow_seekable_input_stream_get_raw(input_stream);

  std::shared_ptr<arrow::Buffer> arrow_buffer;
  auto status = arrow_random_access_file->ReadAt(position,
                                                 n_bytes,
                                                 &arrow_buffer);
  if (garrow_error_check(error, status, "[seekable-input-stream][read-at]")) {
    return garrow_buffer_new_raw(&arrow_buffer);
  } else {
    return NULL;
  }
}


G_DEFINE_TYPE(GArrowBufferInputStream,                       \
              garrow_buffer_input_stream,                     \
              GARROW_TYPE_SEEKABLE_INPUT_STREAM);

static void
garrow_buffer_input_stream_init(GArrowBufferInputStream *object)
{
}

static void
garrow_buffer_input_stream_class_init(GArrowBufferInputStreamClass *klass)
{
}

/**
 * garrow_buffer_input_stream_new:
 * @buffer: The buffer to be read.
 *
 * Returns: A newly created #GArrowBufferInputStream.
 */
GArrowBufferInputStream *
garrow_buffer_input_stream_new(GArrowBuffer *buffer)
{
  auto arrow_buffer = garrow_buffer_get_raw(buffer);
  auto arrow_buffer_reader =
    std::make_shared<arrow::io::BufferReader>(arrow_buffer);
  return garrow_buffer_input_stream_new_raw(&arrow_buffer_reader);
}

/**
 * garrow_buffer_input_stream_get_buffer:
 * @input_stream: A #GArrowBufferInputStream.
 *
 * Returns: (transfer full): The data of the array as #GArrowBuffer.
 */
GArrowBuffer *
garrow_buffer_input_stream_get_buffer(GArrowBufferInputStream *input_stream)
{
  auto arrow_buffer_reader = garrow_buffer_input_stream_get_raw(input_stream);
  auto arrow_buffer = arrow_buffer_reader->buffer();
  return garrow_buffer_new_raw(&arrow_buffer);
}


G_DEFINE_TYPE(GArrowMemoryMappedInputStream,            \
              garrow_memory_mapped_input_stream,        \
              GARROW_TYPE_SEEKABLE_INPUT_STREAM);

static void
garrow_memory_mapped_input_stream_init(GArrowMemoryMappedInputStream *object)
{
}

static void
garrow_memory_mapped_input_stream_class_init(GArrowMemoryMappedInputStreamClass *klass)
{
}

/**
 * garrow_memory_mapped_input_stream_new:
 * @path: The path of the file to be mapped on memory.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: (nullable): A newly created #GArrowMemoryMappedInputStream
 *   or %NULL on error.
 */
GArrowMemoryMappedInputStream *
garrow_memory_mapped_input_stream_new(const gchar *path,
                                      GError **error)
{
  std::shared_ptr<arrow::io::MemoryMappedFile> arrow_memory_mapped_file;
  auto status =
    arrow::io::MemoryMappedFile::Open(std::string(path),
                                      arrow::io::FileMode::READ,
                                      &arrow_memory_mapped_file);
  if (status.ok()) {
    return garrow_memory_mapped_input_stream_new_raw(&arrow_memory_mapped_file);
  } else {
    std::string context("[memory-mapped-input-stream][open]: <");
    context += path;
    context += ">";
    garrow_error_check(error, status, context.c_str());
    return NULL;
  }
}


G_END_DECLS

GArrowInputStream *
garrow_input_stream_new_raw(std::shared_ptr<arrow::io::InputStream> *arrow_input_stream)
{
  auto input_stream =
    GARROW_INPUT_STREAM(g_object_new(GARROW_TYPE_INPUT_STREAM,
                                     "input-stream", arrow_input_stream,
                                     NULL));
  return input_stream;
}

std::shared_ptr<arrow::io::InputStream>
garrow_input_stream_get_raw(GArrowInputStream *input_stream)
{
  GArrowInputStreamPrivate *priv;

  priv = GARROW_INPUT_STREAM_GET_PRIVATE(input_stream);
  return priv->input_stream;
}

std::shared_ptr<arrow::io::RandomAccessFile>
garrow_seekable_input_stream_get_raw(GArrowSeekableInputStream *seekable_input_stream)
{
  auto arrow_input_stream =
    garrow_input_stream_get_raw(GARROW_INPUT_STREAM(seekable_input_stream));
  auto arrow_random_access_file =
    std::static_pointer_cast<arrow::io::RandomAccessFile>(arrow_input_stream);
  return arrow_random_access_file;
}

GArrowBufferInputStream *
garrow_buffer_input_stream_new_raw(std::shared_ptr<arrow::io::BufferReader> *arrow_buffer_reader)
{
  auto buffer_input_stream =
    GARROW_BUFFER_INPUT_STREAM(g_object_new(GARROW_TYPE_BUFFER_INPUT_STREAM,
                                            "input-stream", arrow_buffer_reader,
                                            NULL));
  return buffer_input_stream;
}

std::shared_ptr<arrow::io::BufferReader>
garrow_buffer_input_stream_get_raw(GArrowBufferInputStream *buffer_input_stream)
{
  auto arrow_input_stream =
    garrow_input_stream_get_raw(GARROW_INPUT_STREAM(buffer_input_stream));
  auto arrow_buffer_reader =
    std::static_pointer_cast<arrow::io::BufferReader>(arrow_input_stream);
  return arrow_buffer_reader;
}

GArrowMemoryMappedInputStream *
garrow_memory_mapped_input_stream_new_raw(std::shared_ptr<arrow::io::MemoryMappedFile> *arrow_memory_mapped_file)
{
  auto object = g_object_new(GARROW_TYPE_MEMORY_MAPPED_INPUT_STREAM,
                             "input-stream", arrow_memory_mapped_file,
                             NULL);
  auto memory_mapped_input_stream = GARROW_MEMORY_MAPPED_INPUT_STREAM(object);
  return memory_mapped_input_stream;
}
