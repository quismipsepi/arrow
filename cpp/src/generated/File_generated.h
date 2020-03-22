// automatically generated by the FlatBuffers compiler, do not modify


#pragma once

#include "flatbuffers/flatbuffers.h"

#include "Schema_generated.h"

namespace org {
namespace apache {
namespace arrow {
namespace flatbuf {

struct Footer;
struct FooterBuilder;

struct Block;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(8) Block FLATBUFFERS_FINAL_CLASS {
 private:
  int64_t offset_;
  int32_t metaDataLength_;
  int32_t padding0__;
  int64_t bodyLength_;

 public:
  Block() {
    memset(static_cast<void *>(this), 0, sizeof(Block));
  }
  Block(int64_t _offset, int32_t _metaDataLength, int64_t _bodyLength)
      : offset_(flatbuffers::EndianScalar(_offset)),
        metaDataLength_(flatbuffers::EndianScalar(_metaDataLength)),
        padding0__(0),
        bodyLength_(flatbuffers::EndianScalar(_bodyLength)) {
    (void)padding0__;
  }
  /// Index to the start of the RecordBlock (note this is past the Message header)
  int64_t offset() const {
    return flatbuffers::EndianScalar(offset_);
  }
  /// Length of the metadata
  int32_t metaDataLength() const {
    return flatbuffers::EndianScalar(metaDataLength_);
  }
  /// Length of the data (this is aligned so there can be a gap between this and
  /// the metatdata).
  int64_t bodyLength() const {
    return flatbuffers::EndianScalar(bodyLength_);
  }
};
FLATBUFFERS_STRUCT_END(Block, 24);

/// ----------------------------------------------------------------------
/// Arrow File metadata
///
struct Footer FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef FooterBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_VERSION = 4,
    VT_SCHEMA = 6,
    VT_DICTIONARIES = 8,
    VT_RECORDBATCHES = 10,
    VT_CUSTOM_METADATA = 12
  };
  org::apache::arrow::flatbuf::MetadataVersion version() const {
    return static_cast<org::apache::arrow::flatbuf::MetadataVersion>(GetField<int16_t>(VT_VERSION, 0));
  }
  const org::apache::arrow::flatbuf::Schema *schema() const {
    return GetPointer<const org::apache::arrow::flatbuf::Schema *>(VT_SCHEMA);
  }
  const flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *> *dictionaries() const {
    return GetPointer<const flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *> *>(VT_DICTIONARIES);
  }
  const flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *> *recordBatches() const {
    return GetPointer<const flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *> *>(VT_RECORDBATCHES);
  }
  /// User-defined metadata
  const flatbuffers::Vector<flatbuffers::Offset<org::apache::arrow::flatbuf::KeyValue>> *custom_metadata() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<org::apache::arrow::flatbuf::KeyValue>> *>(VT_CUSTOM_METADATA);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int16_t>(verifier, VT_VERSION) &&
           VerifyOffset(verifier, VT_SCHEMA) &&
           verifier.VerifyTable(schema()) &&
           VerifyOffset(verifier, VT_DICTIONARIES) &&
           verifier.VerifyVector(dictionaries()) &&
           VerifyOffset(verifier, VT_RECORDBATCHES) &&
           verifier.VerifyVector(recordBatches()) &&
           VerifyOffset(verifier, VT_CUSTOM_METADATA) &&
           verifier.VerifyVector(custom_metadata()) &&
           verifier.VerifyVectorOfTables(custom_metadata()) &&
           verifier.EndTable();
  }
};

struct FooterBuilder {
  typedef Footer Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_version(org::apache::arrow::flatbuf::MetadataVersion version) {
    fbb_.AddElement<int16_t>(Footer::VT_VERSION, static_cast<int16_t>(version), 0);
  }
  void add_schema(flatbuffers::Offset<org::apache::arrow::flatbuf::Schema> schema) {
    fbb_.AddOffset(Footer::VT_SCHEMA, schema);
  }
  void add_dictionaries(flatbuffers::Offset<flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *>> dictionaries) {
    fbb_.AddOffset(Footer::VT_DICTIONARIES, dictionaries);
  }
  void add_recordBatches(flatbuffers::Offset<flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *>> recordBatches) {
    fbb_.AddOffset(Footer::VT_RECORDBATCHES, recordBatches);
  }
  void add_custom_metadata(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<org::apache::arrow::flatbuf::KeyValue>>> custom_metadata) {
    fbb_.AddOffset(Footer::VT_CUSTOM_METADATA, custom_metadata);
  }
  explicit FooterBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FooterBuilder &operator=(const FooterBuilder &);
  flatbuffers::Offset<Footer> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Footer>(end);
    return o;
  }
};

inline flatbuffers::Offset<Footer> CreateFooter(
    flatbuffers::FlatBufferBuilder &_fbb,
    org::apache::arrow::flatbuf::MetadataVersion version = org::apache::arrow::flatbuf::MetadataVersion::V1,
    flatbuffers::Offset<org::apache::arrow::flatbuf::Schema> schema = 0,
    flatbuffers::Offset<flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *>> dictionaries = 0,
    flatbuffers::Offset<flatbuffers::Vector<const org::apache::arrow::flatbuf::Block *>> recordBatches = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<org::apache::arrow::flatbuf::KeyValue>>> custom_metadata = 0) {
  FooterBuilder builder_(_fbb);
  builder_.add_custom_metadata(custom_metadata);
  builder_.add_recordBatches(recordBatches);
  builder_.add_dictionaries(dictionaries);
  builder_.add_schema(schema);
  builder_.add_version(version);
  return builder_.Finish();
}

inline flatbuffers::Offset<Footer> CreateFooterDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    org::apache::arrow::flatbuf::MetadataVersion version = org::apache::arrow::flatbuf::MetadataVersion::V1,
    flatbuffers::Offset<org::apache::arrow::flatbuf::Schema> schema = 0,
    const std::vector<org::apache::arrow::flatbuf::Block> *dictionaries = nullptr,
    const std::vector<org::apache::arrow::flatbuf::Block> *recordBatches = nullptr,
    const std::vector<flatbuffers::Offset<org::apache::arrow::flatbuf::KeyValue>> *custom_metadata = nullptr) {
  auto dictionaries__ = dictionaries ? _fbb.CreateVectorOfStructs<org::apache::arrow::flatbuf::Block>(*dictionaries) : 0;
  auto recordBatches__ = recordBatches ? _fbb.CreateVectorOfStructs<org::apache::arrow::flatbuf::Block>(*recordBatches) : 0;
  auto custom_metadata__ = custom_metadata ? _fbb.CreateVector<flatbuffers::Offset<org::apache::arrow::flatbuf::KeyValue>>(*custom_metadata) : 0;
  return org::apache::arrow::flatbuf::CreateFooter(
      _fbb,
      version,
      schema,
      dictionaries__,
      recordBatches__,
      custom_metadata__);
}

inline const org::apache::arrow::flatbuf::Footer *GetFooter(const void *buf) {
  return flatbuffers::GetRoot<org::apache::arrow::flatbuf::Footer>(buf);
}

inline const org::apache::arrow::flatbuf::Footer *GetSizePrefixedFooter(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<org::apache::arrow::flatbuf::Footer>(buf);
}

inline bool VerifyFooterBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<org::apache::arrow::flatbuf::Footer>(nullptr);
}

inline bool VerifySizePrefixedFooterBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<org::apache::arrow::flatbuf::Footer>(nullptr);
}

inline void FinishFooterBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<org::apache::arrow::flatbuf::Footer> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedFooterBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<org::apache::arrow::flatbuf::Footer> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace flatbuf
}  // namespace arrow
}  // namespace apache
}  // namespace org

