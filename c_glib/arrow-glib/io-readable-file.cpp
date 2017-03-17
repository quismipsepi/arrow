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

#include <arrow/api.h>

#include <arrow-glib/error.hpp>
#include <arrow-glib/io-readable-file.hpp>

G_BEGIN_DECLS

/**
 * SECTION: io-readable-file
 * @title: GArrowIOReadableFile
 * @short_description: File input interface
 *
 * #GArrowIOReadableFile is an interface for file input.
 */

G_DEFINE_INTERFACE(GArrowIOReadableFile,
                   garrow_io_readable_file,
                   G_TYPE_OBJECT)

static void
garrow_io_readable_file_default_init (GArrowIOReadableFileInterface *iface)
{
}

/**
 * garrow_io_readable_file_get_size:
 * @file: A #GArrowIOReadableFile.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: The size of the file.
 */
guint64
garrow_io_readable_file_get_size(GArrowIOReadableFile *file,
                                 GError **error)
{
  auto *iface = GARROW_IO_READABLE_FILE_GET_IFACE(file);
  auto arrow_readable_file = iface->get_raw(file);
  int64_t size;

  auto status = arrow_readable_file->GetSize(&size);
  if (status.ok()) {
    return size;
  } else {
    garrow_error_set(error, status, "[io][readable-file][get-size]");
    return 0;
  }
}

/**
 * garrow_io_readable_file_get_support_zero_copy:
 * @file: A #GArrowIOReadableFile.
 *
 * Returns: Whether zero copy read is supported or not.
 */
gboolean
garrow_io_readable_file_get_support_zero_copy(GArrowIOReadableFile *file)
{
  auto *iface = GARROW_IO_READABLE_FILE_GET_IFACE(file);
  auto arrow_readable_file = iface->get_raw(file);

  return arrow_readable_file->supports_zero_copy();
}

/**
 * garrow_io_readable_file_read_at:
 * @file: A #GArrowIOReadableFile.
 * @position: The read start position.
 * @n_bytes: The number of bytes to be read.
 * @n_read_bytes: (out): The read number of bytes.
 * @buffer: (array length=n_bytes): The buffer to be read data.
 * @error: (nullable): Return location for a #GError or %NULL.
 *
 * Returns: %TRUE on success, %FALSE if there was an error.
 */
gboolean
garrow_io_readable_file_read_at(GArrowIOReadableFile *file,
                                gint64 position,
                                gint64 n_bytes,
                                gint64 *n_read_bytes,
                                guint8 *buffer,
                                GError **error)
{
  const auto arrow_readable_file = garrow_io_readable_file_get_raw(file);

  auto status = arrow_readable_file->ReadAt(position,
                                            n_bytes,
                                            n_read_bytes,
                                            buffer);
  if (status.ok()) {
    return TRUE;
  } else {
    garrow_error_set(error, status, "[io][readable-file][read-at]");
    return FALSE;
  }
}

G_END_DECLS

std::shared_ptr<arrow::io::ReadableFileInterface>
garrow_io_readable_file_get_raw(GArrowIOReadableFile *readable_file)
{
  auto *iface = GARROW_IO_READABLE_FILE_GET_IFACE(readable_file);
  return iface->get_raw(readable_file);
}
