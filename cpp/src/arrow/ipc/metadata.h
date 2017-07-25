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

// C++ object model and user API for interprocess schema messaging

#ifndef ARROW_IPC_METADATA_H
#define ARROW_IPC_METADATA_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "arrow/util/macros.h"
#include "arrow/util/visibility.h"

namespace arrow {

class Array;
class Buffer;
class DataType;
class Field;
class Schema;
class Status;
class Tensor;

namespace io {

class InputStream;
class OutputStream;
class RandomAccessFile;

}  // namespace io

namespace ipc {

enum class MetadataVersion : char { V1, V2, V3 };

static constexpr const char* kArrowMagicBytes = "ARROW1";

// ARROW-109: We set this number arbitrarily to help catch user mistakes. For
// deeply nested schemas, it is expected the user will indicate explicitly the
// maximum allowed recursion depth
constexpr int kMaxNestingDepth = 64;

struct ARROW_EXPORT FieldMetadata {
  int64_t length;
  int64_t null_count;
  int64_t offset;
};

struct ARROW_EXPORT BufferMetadata {
  /// The shared memory page id where to find this. Set to -1 if unused
  int32_t page;

  /// The relative offset into the memory page to the starting byte of the buffer
  int64_t offset;

  /// Absolute length in bytes of the buffer
  int64_t length;
};

struct FileBlock {
  int64_t offset;
  int32_t metadata_length;
  int64_t body_length;
};

//----------------------------------------------------------------------

using DictionaryMap = std::unordered_map<int64_t, std::shared_ptr<Array>>;
using DictionaryTypeMap = std::unordered_map<int64_t, std::shared_ptr<Field>>;

// Memoization data structure for handling shared dictionaries
class ARROW_EXPORT DictionaryMemo {
 public:
  DictionaryMemo();

  // Returns KeyError if dictionary not found
  Status GetDictionary(int64_t id, std::shared_ptr<Array>* dictionary) const;

  /// Return id for dictionary, computing new id if necessary
  int64_t GetId(const std::shared_ptr<Array>& dictionary);

  bool HasDictionary(const std::shared_ptr<Array>& dictionary) const;
  bool HasDictionaryId(int64_t id) const;

  // Add a dictionary to the memo with a particular id. Returns KeyError if
  // that dictionary already exists
  Status AddDictionary(int64_t id, const std::shared_ptr<Array>& dictionary);

  const DictionaryMap& id_to_dictionary() const { return id_to_dictionary_; }

  int size() const { return static_cast<int>(id_to_dictionary_.size()); }

 private:
  // Dictionary memory addresses, to track whether a dictionary has been seen
  // before
  std::unordered_map<intptr_t, int64_t> dictionary_to_id_;

  // Map of dictionary id to dictionary array
  DictionaryMap id_to_dictionary_;

  DISALLOW_COPY_AND_ASSIGN(DictionaryMemo);
};

// Read interface classes. We do not fully deserialize the flatbuffers so that
// individual fields metadata can be retrieved from very large schema without
//

class Message;

// Retrieve a list of all the dictionary ids and types required by the schema for
// reconstruction. The presumption is that these will be loaded either from
// the stream or file (or they may already be somewhere else in memory)
Status GetDictionaryTypes(const void* opaque_schema, DictionaryTypeMap* id_to_field);

// Construct a complete Schema from the message. May be expensive for very
// large schemas if you are only interested in a few fields
Status ARROW_EXPORT GetSchema(const void* opaque_schema,
                              const DictionaryMemo& dictionary_memo,
                              std::shared_ptr<Schema>* out);

Status ARROW_EXPORT GetTensorMetadata(const Buffer& metadata,
                                      std::shared_ptr<DataType>* type,
                                      std::vector<int64_t>* shape,
                                      std::vector<int64_t>* strides,
                                      std::vector<std::string>* dim_names);

/// \brief An IPC message including metadata and body
class ARROW_EXPORT Message {
 public:
  enum Type { NONE, SCHEMA, DICTIONARY_BATCH, RECORD_BATCH, TENSOR };

  /// \brief Construct message, but do not validate
  ///
  /// Use at your own risk; Message::Open has more metadata validation
  Message(const std::shared_ptr<Buffer>& metadata, const std::shared_ptr<Buffer>& body);

  ~Message();

  /// \brief Create and validate a Message instance from two buffers
  ///
  /// \param[in] metadata a buffer containing the Flatbuffer metadata
  /// \param[in] body a buffer containing the message body, which may be nullptr
  /// \param[out] out the created message
  static Status Open(const std::shared_ptr<Buffer>& metadata,
                     const std::shared_ptr<Buffer>& body, std::unique_ptr<Message>* out);

  /// \brief Write length-prefixed metadata and body to output stream
  ///
  /// \param[in] file output stream to write to
  /// \param[out] output_length the number of bytes written
  /// \return Status
  bool Equals(const Message& other) const;

  /// \brief the Message metadata
  ///
  /// \return buffer
  std::shared_ptr<Buffer> metadata() const;

  /// \brief the Message body, if any
  ///
  /// \return buffer is nullptr if no body
  std::shared_ptr<Buffer> body() const;

  Type type() const;

  MetadataVersion metadata_version() const;

  const void* header() const;

  /// \brief Write length-prefixed metadata and body to output stream
  ///
  /// \param[in] file output stream to write to
  /// \param[out] output_length the number of bytes written
  /// \return Status
  Status SerializeTo(io::OutputStream* file, int64_t* output_length) const;

 private:
  // Hide serialization details from user API
  class MessageImpl;
  std::unique_ptr<MessageImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(Message);
};

ARROW_EXPORT std::string FormatMessageType(Message::Type type);

/// \brief Abstract interface for a sequence of messages
/// \since 0.5.0
class ARROW_EXPORT MessageReader {
 public:
  virtual ~MessageReader() = default;

  /// \brief Read next Message from the interface
  ///
  /// \param[out] message an arrow::ipc::Message instance
  /// \return Status
  virtual Status ReadNextMessage(std::unique_ptr<Message>* message) = 0;
};

/// \brief Implementation of MessageReader that reads from InputStream
/// \since 0.5.0
class ARROW_EXPORT InputStreamMessageReader : public MessageReader {
 public:
  explicit InputStreamMessageReader(const std::shared_ptr<io::InputStream>& stream)
      : stream_(stream) {}

  ~InputStreamMessageReader();

  Status ReadNextMessage(std::unique_ptr<Message>* message) override;

 private:
  std::shared_ptr<io::InputStream> stream_;
};

/// \brief Read encapulated RPC message from position in file
///
/// Read a length-prefixed message flatbuffer starting at the indicated file
/// offset. If the message has a body with non-zero length, it will also be
/// read
///
/// The metadata_length includes at least the length prefix and the flatbuffer
///
/// \param[in] offset the position in the file where the message starts. The
/// first 4 bytes after the offset are the message length
/// \param[in] metadata_length the total number of bytes to read from file
/// \param[in] file the seekable file interface to read from
/// \param[out] message the message read
/// \return Status success or failure
Status ARROW_EXPORT ReadMessage(int64_t offset, int32_t metadata_length,
                                io::RandomAccessFile* file,
                                std::unique_ptr<Message>* message);

/// \brief Read encapulated RPC message (metadata and body) from InputStream
///
/// Read length-prefixed message with as-yet unknown length. Returns nullptr if
/// there are not enough bytes available or the message length is 0 (e.g. EOS
/// in a stream)
Status ARROW_EXPORT ReadMessage(io::InputStream* stream,
                                std::unique_ptr<Message>* message);

/// Write a serialized message metadata with a length-prefix and padding to an
/// 8-byte offset
///
/// <message_size: int32><message: const void*><padding>
Status ARROW_EXPORT WriteMessage(const Buffer& message, io::OutputStream* file,
                                 int32_t* message_length);

// Serialize arrow::Schema as a Flatbuffer
//
// \param[in] schema a Schema instance
// \param[inout] dictionary_memo class for tracking dictionaries and assigning
// dictionary ids
// \param[out] out the serialized arrow::Buffer
// \return Status outcome
Status ARROW_EXPORT WriteSchemaMessage(const Schema& schema,
                                       DictionaryMemo* dictionary_memo,
                                       std::shared_ptr<Buffer>* out);

Status ARROW_EXPORT WriteRecordBatchMessage(int64_t length, int64_t body_length,
                                            const std::vector<FieldMetadata>& nodes,
                                            const std::vector<BufferMetadata>& buffers,
                                            std::shared_ptr<Buffer>* out);

Status ARROW_EXPORT WriteTensorMessage(const Tensor& tensor, int64_t buffer_start_offset,
                                       std::shared_ptr<Buffer>* out);

Status WriteDictionaryMessage(int64_t id, int64_t length, int64_t body_length,
                              const std::vector<FieldMetadata>& nodes,
                              const std::vector<BufferMetadata>& buffers,
                              std::shared_ptr<Buffer>* out);

Status WriteFileFooter(const Schema& schema, const std::vector<FileBlock>& dictionaries,
                       const std::vector<FileBlock>& record_batches,
                       DictionaryMemo* dictionary_memo, io::OutputStream* out);

}  // namespace ipc
}  // namespace arrow

#endif  // ARROW_IPC_METADATA_H
