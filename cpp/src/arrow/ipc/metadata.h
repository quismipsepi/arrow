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
#include <unordered_map>
#include <vector>

#include "arrow/loader.h"
#include "arrow/util/macros.h"
#include "arrow/util/visibility.h"

namespace arrow {

class Array;
class Buffer;
struct DataType;
struct Field;
class Schema;
class Status;

namespace io {

class OutputStream;
class RandomAccessFile;

}  // namespace io

namespace ipc {

struct MetadataVersion {
  enum type { V1, V2 };
};

static constexpr const char* kArrowMagicBytes = "ARROW1";

struct ARROW_EXPORT FileBlock {
  FileBlock() {}
  FileBlock(int64_t offset, int32_t metadata_length, int64_t body_length)
      : offset(offset), metadata_length(metadata_length), body_length(body_length) {}

  int64_t offset;
  int32_t metadata_length;
  int64_t body_length;
};

//----------------------------------------------------------------------

using DictionaryMap = std::unordered_map<int64_t, std::shared_ptr<Array>>;
using DictionaryTypeMap = std::unordered_map<int64_t, std::shared_ptr<Field>>;

// Memoization data structure for handling shared dictionaries
class DictionaryMemo {
 public:
  DictionaryMemo();

  // Returns KeyError if dictionary not found
  Status GetDictionary(int64_t id, std::shared_ptr<Array>* dictionary) const;

  int64_t GetId(const std::shared_ptr<Array>& dictionary);

  bool HasDictionary(const std::shared_ptr<Array>& dictionary) const;
  bool HasDictionaryId(int64_t id) const;

  // Add a dictionary to the memo with a particular id. Returns KeyError if
  // that dictionary already exists
  Status AddDictionary(int64_t id, const std::shared_ptr<Array>& dictionary);

  const DictionaryMap& id_to_dictionary() const { return id_to_dictionary_; }

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

// Container for serialized Schema metadata contained in an IPC message
class ARROW_EXPORT SchemaMetadata {
 public:
  explicit SchemaMetadata(const void* header);
  explicit SchemaMetadata(const std::shared_ptr<Message>& message);
  SchemaMetadata(const std::shared_ptr<Buffer>& message, int64_t offset);

  ~SchemaMetadata();

  int num_fields() const;

  // Retrieve a list of all the dictionary ids and types required by the schema for
  // reconstruction. The presumption is that these will be loaded either from
  // the stream or file (or they may already be somewhere else in memory)
  Status GetDictionaryTypes(DictionaryTypeMap* id_to_field) const;

  // Construct a complete Schema from the message. May be expensive for very
  // large schemas if you are only interested in a few fields
  Status GetSchema(
      const DictionaryMemo& dictionary_memo, std::shared_ptr<Schema>* out) const;

 private:
  class SchemaMetadataImpl;
  std::unique_ptr<SchemaMetadataImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(SchemaMetadata);
};

struct ARROW_EXPORT BufferMetadata {
  int32_t page;
  int64_t offset;
  int64_t length;
};

class ARROW_EXPORT Message {
 public:
  ~Message();
  enum Type { NONE, SCHEMA, DICTIONARY_BATCH, RECORD_BATCH };

  static Status Open(const std::shared_ptr<Buffer>& buffer, int64_t offset,
      std::shared_ptr<Message>* out);

  int64_t body_length() const;

  Type type() const;

  const void* header() const;

 private:
  Message(const std::shared_ptr<Buffer>& buffer, int64_t offset);

  friend class DictionaryBatchMetadata;
  friend class SchemaMetadata;

  // Hide serialization details from user API
  class MessageImpl;
  std::unique_ptr<MessageImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(Message);
};

/// Read a length-prefixed message flatbuffer starting at the indicated file
/// offset
///
/// The metadata_length includes at least the length prefix and the flatbuffer
///
/// \param[in] offset the position in the file where the message starts. The
/// first 4 bytes after the offset are the message length
/// \param[in] metadata_length the total number of bytes to read from file
/// \param[in] file the seekable file interface to read from
/// \param[out] message the message read
/// \return Status success or failure
Status ReadMessage(int64_t offset, int32_t metadata_length, io::RandomAccessFile* file,
    std::shared_ptr<Message>* message);

// Serialize arrow::Schema as a Flatbuffer
//
// \param[in] schema a Schema instance
// \param[inout] dictionary_memo class for tracking dictionaries and assigning
// dictionary ids
// \param[out] out the serialized arrow::Buffer
// \return Status outcome
Status WriteSchemaMessage(
    const Schema& schema, DictionaryMemo* dictionary_memo, std::shared_ptr<Buffer>* out);

Status WriteRecordBatchMessage(int64_t length, int64_t body_length,
    const std::vector<FieldMetadata>& nodes, const std::vector<BufferMetadata>& buffers,
    std::shared_ptr<Buffer>* out);

Status WriteDictionaryMessage(int64_t id, int64_t length, int64_t body_length,
    const std::vector<FieldMetadata>& nodes, const std::vector<BufferMetadata>& buffers,
    std::shared_ptr<Buffer>* out);

Status WriteFileFooter(const Schema& schema, const std::vector<FileBlock>& dictionaries,
    const std::vector<FileBlock>& record_batches, DictionaryMemo* dictionary_memo,
    io::OutputStream* out);

}  // namespace ipc
}  // namespace arrow

#endif  // ARROW_IPC_METADATA_H
