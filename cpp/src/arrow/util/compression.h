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

#ifndef ARROW_UTIL_COMPRESSION_H
#define ARROW_UTIL_COMPRESSION_H

#include <cstdint>
#include <memory>

#include "arrow/status.h"
#include "arrow/util/visibility.h"

namespace arrow {

struct Compression {
  enum type { UNCOMPRESSED, SNAPPY, GZIP, LZO, BROTLI, ZSTD, LZ4 };
};

class ARROW_EXPORT Codec {
 public:
  virtual ~Codec();

  static Status Create(Compression::type codec, std::unique_ptr<Codec>* out);

  virtual Status Decompress(int64_t input_len, const uint8_t* input, int64_t output_len,
      uint8_t* output_buffer) = 0;

  virtual Status Compress(int64_t input_len, const uint8_t* input,
      int64_t output_buffer_len, uint8_t* output_buffer, int64_t* output_length) = 0;

  virtual int64_t MaxCompressedLen(int64_t input_len, const uint8_t* input) = 0;

  virtual const char* name() const = 0;
};

// Snappy codec.
class ARROW_EXPORT SnappyCodec : public Codec {
 public:
  Status Decompress(int64_t input_len, const uint8_t* input, int64_t output_len,
      uint8_t* output_buffer) override;

  Status Compress(int64_t input_len, const uint8_t* input, int64_t output_buffer_len,
      uint8_t* output_buffer, int64_t* output_length) override;

  int64_t MaxCompressedLen(int64_t input_len, const uint8_t* input) override;

  const char* name() const override { return "snappy"; }
};

// Brotli codec.
class ARROW_EXPORT BrotliCodec : public Codec {
 public:
  Status Decompress(int64_t input_len, const uint8_t* input, int64_t output_len,
      uint8_t* output_buffer) override;

  Status Compress(int64_t input_len, const uint8_t* input, int64_t output_buffer_len,
      uint8_t* output_buffer, int64_t* output_length) override;

  int64_t MaxCompressedLen(int64_t input_len, const uint8_t* input) override;

  const char* name() const override { return "brotli"; }
};

// GZip codec.
class ARROW_EXPORT GZipCodec : public Codec {
 public:
  /// Compression formats supported by the zlib library
  enum Format {
    ZLIB,
    DEFLATE,
    GZIP,
  };

  explicit GZipCodec(Format format = GZIP);
  virtual ~GZipCodec();

  Status Decompress(int64_t input_len, const uint8_t* input, int64_t output_len,
      uint8_t* output_buffer) override;

  Status Compress(int64_t input_len, const uint8_t* input, int64_t output_buffer_len,
      uint8_t* output_buffer, int64_t* output_length) override;

  int64_t MaxCompressedLen(int64_t input_len, const uint8_t* input) override;

  const char* name() const override;

 private:
  // The gzip compressor is stateful
  class GZipCodecImpl;
  std::unique_ptr<GZipCodecImpl> impl_;
};

// ZSTD codec.
class ARROW_EXPORT ZSTDCodec : public Codec {
 public:
  Status Decompress(int64_t input_len, const uint8_t* input, int64_t output_len,
      uint8_t* output_buffer) override;

  Status Compress(int64_t input_len, const uint8_t* input, int64_t output_buffer_len,
      uint8_t* output_buffer, int64_t* output_length) override;

  int64_t MaxCompressedLen(int64_t input_len, const uint8_t* input) override;

  const char* name() const override { return "zstd"; }
};

// Lz4 codec.
class ARROW_EXPORT Lz4Codec : public Codec {
 public:
  Status Decompress(int64_t input_len, const uint8_t* input, int64_t output_len,
      uint8_t* output_buffer) override;

  Status Compress(int64_t input_len, const uint8_t* input, int64_t output_buffer_len,
      uint8_t* output_buffer, int64_t* output_length) override;

  int64_t MaxCompressedLen(int64_t input_len, const uint8_t* input) override;

  const char* name() const override { return "lz4"; }
};

}  // namespace arrow

#endif
