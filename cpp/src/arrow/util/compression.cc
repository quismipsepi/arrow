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

#include "arrow/util/compression.h"

// Work around warning caused by Snappy include
#ifdef DISALLOW_COPY_AND_ASSIGN
#undef DISALLOW_COPY_AND_ASSIGN
#endif

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include <brotli/decode.h>
#include <brotli/encode.h>
#include <lz4.h>
#include <snappy.h>
#include <zlib.h>
#include <zstd.h>

#include "arrow/status.h"
#include "arrow/util/logging.h"

namespace arrow {

Codec::~Codec() {}

Status Codec::Create(Compression::type codec_type, std::unique_ptr<Codec>* result) {
  switch (codec_type) {
    case Compression::UNCOMPRESSED:
      break;
    case Compression::SNAPPY:
      result->reset(new SnappyCodec());
      break;
    case Compression::GZIP:
      result->reset(new GZipCodec());
      break;
    case Compression::LZO:
      return Status::NotImplemented("LZO codec not implemented");
    case Compression::BROTLI:
      result->reset(new BrotliCodec());
      break;
    default:
      return Status::Invalid("Unrecognized codec");
  }
  return Status::OK();
}

// ----------------------------------------------------------------------
// gzip implementation

// These are magic numbers from zlib.h.  Not clear why they are not defined
// there.

// Maximum window size
static constexpr int WINDOW_BITS = 15;

// Output Gzip.
static constexpr int GZIP_CODEC = 16;

// Determine if this is libz or gzip from header.
static constexpr int DETECT_CODEC = 32;

class GZipCodec::GZipCodecImpl {
 public:
  explicit GZipCodecImpl(GZipCodec::Format format)
      : format_(format),
        compressor_initialized_(false),
        decompressor_initialized_(false) {}

  ~GZipCodecImpl() {
    EndCompressor();
    EndDecompressor();
  }

  Status InitCompressor() {
    EndDecompressor();
    memset(&stream_, 0, sizeof(stream_));

    int ret;
    // Initialize to run specified format
    int window_bits = WINDOW_BITS;
    if (format_ == DEFLATE) {
      window_bits = -window_bits;
    } else if (format_ == GZIP) {
      window_bits += GZIP_CODEC;
    }
    if ((ret = deflateInit2(&stream_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits, 9,
             Z_DEFAULT_STRATEGY)) != Z_OK) {
      std::stringstream ss;
      ss << "zlib deflateInit failed: " << std::string(stream_.msg);
      return Status::IOError(ss.str());
    }
    compressor_initialized_ = true;
    return Status::OK();
  }

  void EndCompressor() {
    if (compressor_initialized_) { (void)deflateEnd(&stream_); }
    compressor_initialized_ = false;
  }

  Status InitDecompressor() {
    EndCompressor();
    memset(&stream_, 0, sizeof(stream_));
    int ret;

    // Initialize to run either deflate or zlib/gzip format
    int window_bits = format_ == DEFLATE ? -WINDOW_BITS : WINDOW_BITS | DETECT_CODEC;
    if ((ret = inflateInit2(&stream_, window_bits)) != Z_OK) {
      std::stringstream ss;
      ss << "zlib inflateInit failed: " << std::string(stream_.msg);
      return Status::IOError(ss.str());
    }
    decompressor_initialized_ = true;
    return Status::OK();
  }

  void EndDecompressor() {
    if (decompressor_initialized_) { (void)inflateEnd(&stream_); }
    decompressor_initialized_ = false;
  }

  Status Decompress(int64_t input_length, const uint8_t* input, int64_t output_length,
      uint8_t* output) {
    if (!decompressor_initialized_) { RETURN_NOT_OK(InitDecompressor()); }
    if (output_length == 0) {
      // The zlib library does not allow *output to be NULL, even when output_length
      // is 0 (inflate() will return Z_STREAM_ERROR). We don't consider this an
      // error, so bail early if no output is expected. Note that we don't signal
      // an error if the input actually contains compressed data.
      return Status::OK();
    }

    // Reset the stream for this block
    if (inflateReset(&stream_) != Z_OK) {
      std::stringstream ss;
      ss << "zlib inflateReset failed: " << std::string(stream_.msg);
      return Status::IOError(ss.str());
    }

    int ret = 0;
    // gzip can run in streaming mode or non-streaming mode.  We only
    // support the non-streaming use case where we present it the entire
    // compressed input and a buffer big enough to contain the entire
    // compressed output.  In the case where we don't know the output,
    // we just make a bigger buffer and try the non-streaming mode
    // from the beginning again.
    while (ret != Z_STREAM_END) {
      stream_.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input));
      stream_.avail_in = static_cast<uInt>(input_length);
      stream_.next_out = reinterpret_cast<Bytef*>(output);
      stream_.avail_out = static_cast<uInt>(output_length);

      // We know the output size.  In this case, we can use Z_FINISH
      // which is more efficient.
      ret = inflate(&stream_, Z_FINISH);
      if (ret == Z_STREAM_END || ret != Z_OK) break;

      // Failure, buffer was too small
      std::stringstream ss;
      ss << "Too small a buffer passed to GZipCodec. InputLength=" << input_length
         << " OutputLength=" << output_length;
      return Status::IOError(ss.str());
    }

    // Failure for some other reason
    if (ret != Z_STREAM_END) {
      std::stringstream ss;
      ss << "GZipCodec failed: ";
      if (stream_.msg != NULL) ss << stream_.msg;
      return Status::IOError(ss.str());
    }
    return Status::OK();
  }

  int64_t MaxCompressedLen(int64_t input_length, const uint8_t* input) {
    // Most be in compression mode
    if (!compressor_initialized_) {
      Status s = InitCompressor();
      DCHECK(s.ok());
    }
    // TODO(wesm): deal with zlib < 1.2.3 (see Impala codebase)
    return deflateBound(&stream_, static_cast<uLong>(input_length));
  }

  Status Compress(int64_t input_length, const uint8_t* input, int64_t output_buffer_len,
      uint8_t* output, int64_t* output_length) {
    if (!compressor_initialized_) { RETURN_NOT_OK(InitCompressor()); }
    stream_.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input));
    stream_.avail_in = static_cast<uInt>(input_length);
    stream_.next_out = reinterpret_cast<Bytef*>(output);
    stream_.avail_out = static_cast<uInt>(output_buffer_len);

    int64_t ret = 0;
    if ((ret = deflate(&stream_, Z_FINISH)) != Z_STREAM_END) {
      if (ret == Z_OK) {
        // will return Z_OK (and stream.msg NOT set) if stream.avail_out is too
        // small
        return Status::IOError("zlib deflate failed, output buffer too small");
      }
      std::stringstream ss;
      ss << "zlib deflate failed: " << stream_.msg;
      return Status::IOError(ss.str());
    }

    if (deflateReset(&stream_) != Z_OK) {
      std::stringstream ss;
      ss << "zlib deflateReset failed: " << std::string(stream_.msg);
      return Status::IOError(ss.str());
    }

    // Actual output length
    *output_length = output_buffer_len - stream_.avail_out;
    return Status::OK();
  }

 private:
  // zlib is stateful and the z_stream state variable must be initialized
  // before
  z_stream stream_;

  // Realistically, this will always be GZIP, but we leave the option open to
  // configure
  GZipCodec::Format format_;

  // These variables are mutually exclusive. When the codec is in "compressor"
  // state, compressor_initialized_ is true while decompressor_initialized_ is
  // false. When it's decompressing, the opposite is true.
  //
  // Indeed, this is slightly hacky, but the alternative is having separate
  // Compressor and Decompressor classes. If this ever becomes an issue, we can
  // perform the refactoring then
  bool compressor_initialized_;
  bool decompressor_initialized_;
};

GZipCodec::GZipCodec(Format format) {
  impl_.reset(new GZipCodecImpl(format));
}

GZipCodec::~GZipCodec() {}

Status GZipCodec::Decompress(int64_t input_length, const uint8_t* input,
    int64_t output_buffer_len, uint8_t* output) {
  return impl_->Decompress(input_length, input, output_buffer_len, output);
}

int64_t GZipCodec::MaxCompressedLen(int64_t input_length, const uint8_t* input) {
  return impl_->MaxCompressedLen(input_length, input);
}

Status GZipCodec::Compress(int64_t input_length, const uint8_t* input,
    int64_t output_buffer_len, uint8_t* output, int64_t* output_length) {
  return impl_->Compress(input_length, input, output_buffer_len, output, output_length);
}

const char* GZipCodec::name() const {
  return "gzip";
}

// ----------------------------------------------------------------------
// Snappy implementation

Status SnappyCodec::Decompress(
    int64_t input_len, const uint8_t* input, int64_t output_len, uint8_t* output_buffer) {
  if (!snappy::RawUncompress(reinterpret_cast<const char*>(input),
          static_cast<size_t>(input_len), reinterpret_cast<char*>(output_buffer))) {
    return Status::IOError("Corrupt snappy compressed data.");
  }
  return Status::OK();
}

int64_t SnappyCodec::MaxCompressedLen(int64_t input_len, const uint8_t* input) {
  return snappy::MaxCompressedLength(input_len);
}

Status SnappyCodec::Compress(int64_t input_len, const uint8_t* input,
    int64_t output_buffer_len, uint8_t* output_buffer, int64_t* output_length) {
  size_t output_len;
  snappy::RawCompress(reinterpret_cast<const char*>(input),
      static_cast<size_t>(input_len), reinterpret_cast<char*>(output_buffer),
      &output_len);
  *output_length = static_cast<int64_t>(output_len);
  return Status::OK();
}

// ----------------------------------------------------------------------
// Brotli implementation

Status BrotliCodec::Decompress(
    int64_t input_len, const uint8_t* input, int64_t output_len, uint8_t* output_buffer) {
  size_t output_size = output_len;
  if (BrotliDecoderDecompress(input_len, input, &output_size, output_buffer) !=
      BROTLI_DECODER_RESULT_SUCCESS) {
    return Status::IOError("Corrupt brotli compressed data.");
  }
  return Status::OK();
}

int64_t BrotliCodec::MaxCompressedLen(int64_t input_len, const uint8_t* input) {
  return BrotliEncoderMaxCompressedSize(input_len);
}

Status BrotliCodec::Compress(int64_t input_len, const uint8_t* input,
    int64_t output_buffer_len, uint8_t* output_buffer, int64_t* output_length) {
  size_t output_len = output_buffer_len;
  // TODO: Make quality configurable. We use 8 as a default as it is the best
  //       trade-off for Parquet workload
  if (BrotliEncoderCompress(8, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, input_len,
          input, &output_len, output_buffer) == BROTLI_FALSE) {
    return Status::IOError("Brotli compression failure.");
  }
  *output_length = output_len;
  return Status::OK();
}

// ----------------------------------------------------------------------
// ZSTD implementation

Status ZSTDCodec::Decompress(
    int64_t input_len, const uint8_t* input, int64_t output_len, uint8_t* output_buffer) {
  int64_t decompressed_size = ZSTD_decompress(output_buffer,
      static_cast<size_t>(output_len), input, static_cast<size_t>(input_len));
  if (decompressed_size != output_len) {
    return Status::IOError("Corrupt ZSTD compressed data.");
  }
  return Status::OK();
}

int64_t ZSTDCodec::MaxCompressedLen(int64_t input_len, const uint8_t* input) {
  return ZSTD_compressBound(input_len);
}

Status ZSTDCodec::Compress(int64_t input_len, const uint8_t* input,
    int64_t output_buffer_len, uint8_t* output_buffer, int64_t* output_length) {
  *output_length = ZSTD_compress(output_buffer, static_cast<size_t>(output_buffer_len),
      input, static_cast<size_t>(input_len), 1);
  if (ZSTD_isError(*output_length)) {
    return Status::IOError("ZSTD compression failure.");
  }
  return Status::OK();
}

// ----------------------------------------------------------------------
// Lz4 implementation

Status Lz4Codec::Decompress(
    int64_t input_len, const uint8_t* input, int64_t output_len, uint8_t* output_buffer) {
  int64_t decompressed_size = LZ4_decompress_safe(reinterpret_cast<const char*>(input),
      reinterpret_cast<char*>(output_buffer), static_cast<int>(input_len),
      static_cast<int>(output_len));
  if (decompressed_size < 1) { return Status::IOError("Corrupt Lz4 compressed data."); }
  return Status::OK();
}

int64_t Lz4Codec::MaxCompressedLen(int64_t input_len, const uint8_t* input) {
  return LZ4_compressBound(static_cast<int>(input_len));
}

Status Lz4Codec::Compress(int64_t input_len, const uint8_t* input,
    int64_t output_buffer_len, uint8_t* output_buffer, int64_t* output_length) {
  *output_length = LZ4_compress_default(reinterpret_cast<const char*>(input),
      reinterpret_cast<char*>(output_buffer), static_cast<int>(input_len),
      static_cast<int>(output_buffer_len));
  if (*output_length < 1) { return Status::IOError("Lz4 compression failure."); }
  return Status::OK();
}

}  // namespace arrow
