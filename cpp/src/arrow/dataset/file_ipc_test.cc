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

#include "arrow/dataset/file_ipc.h"

#include <memory>
#include <utility>
#include <vector>

#include "arrow/dataset/dataset_internal.h"
#include "arrow/dataset/filter.h"
#include "arrow/dataset/test_util.h"
#include "arrow/io/memory.h"
#include "arrow/ipc/writer.h"
#include "arrow/record_batch.h"
#include "arrow/testing/gtest_util.h"
#include "arrow/testing/util.h"

namespace arrow {
namespace dataset {

constexpr int64_t kBatchSize = 1UL << 12;
constexpr int64_t kBatchRepetitions = 1 << 5;
constexpr int64_t kNumRows = kBatchSize * kBatchRepetitions;

class ArrowIpcWriterMixin : public ::testing::Test {
 public:
  std::shared_ptr<Buffer> Write(RecordBatchReader* reader) {
    EXPECT_OK_AND_ASSIGN(auto sink, io::BufferOutputStream::Create());

    EXPECT_OK_AND_ASSIGN(auto writer,
                         ipc::RecordBatchFileWriter::Open(sink.get(), reader->schema()));

    std::vector<std::shared_ptr<RecordBatch>> batches;
    ARROW_EXPECT_OK(reader->ReadAll(&batches));
    for (auto batch : batches) {
      ARROW_EXPECT_OK(writer->WriteRecordBatch(*batch));
    }

    ARROW_EXPECT_OK(writer->Close());

    EXPECT_OK_AND_ASSIGN(auto out, sink->Finish());
    return out;
  }

  std::shared_ptr<Buffer> Write(const Table& table) {
    EXPECT_OK_AND_ASSIGN(auto sink, io::BufferOutputStream::Create());

    EXPECT_OK_AND_ASSIGN(auto writer,
                         ipc::RecordBatchFileWriter::Open(sink.get(), table.schema()));

    ARROW_EXPECT_OK(writer->WriteTable(table));

    ARROW_EXPECT_OK(writer->Close());

    EXPECT_OK_AND_ASSIGN(auto out, sink->Finish());
    return out;
  }
};

class TestIpcFileFormat : public ArrowIpcWriterMixin {
 public:
  std::unique_ptr<FileSource> GetFileSource(RecordBatchReader* reader) {
    auto buffer = Write(reader);
    return internal::make_unique<FileSource>(std::move(buffer));
  }

  std::unique_ptr<RecordBatchReader> GetRecordBatchReader(
      std::shared_ptr<Schema> schema = nullptr) {
    return MakeGeneratedRecordBatch(schema ? schema : schema_, kBatchSize,
                                    kBatchRepetitions);
  }

  RecordBatchIterator Batches(ScanTaskIterator scan_task_it) {
    return MakeFlattenIterator(MakeMaybeMapIterator(
        [](std::shared_ptr<ScanTask> scan_task) { return scan_task->Execute(); },
        std::move(scan_task_it)));
  }

  RecordBatchIterator Batches(Fragment* fragment) {
    EXPECT_OK_AND_ASSIGN(auto scan_task_it, fragment->Scan(ctx_));
    return Batches(std::move(scan_task_it));
  }

 protected:
  std::shared_ptr<ScanOptions> opts_;
  std::shared_ptr<ScanContext> ctx_ = std::make_shared<ScanContext>();
  std::shared_ptr<Schema> schema_ = schema({field("f64", float64())});
};

TEST_F(TestIpcFileFormat, ScanRecordBatchReader) {
  auto reader = GetRecordBatchReader();
  auto source = GetFileSource(reader.get());

  opts_ = ScanOptions::Make(reader->schema());
  auto fragment = std::make_shared<IpcFragment>(*source, opts_);

  int64_t row_count = 0;

  for (auto maybe_batch : Batches(fragment.get())) {
    ASSERT_OK_AND_ASSIGN(auto batch, std::move(maybe_batch));
    row_count += batch->num_rows();
  }

  ASSERT_EQ(row_count, kNumRows);
}

TEST_F(TestIpcFileFormat, OpenFailureWithRelevantError) {
  auto format = IpcFileFormat();

  std::shared_ptr<Buffer> buf = std::make_shared<Buffer>(util::string_view(""));
  auto result = format.Inspect(FileSource(buf));
  EXPECT_RAISES_WITH_MESSAGE_THAT(Invalid, testing::HasSubstr("<Buffer>"),
                                  result.status());

  constexpr auto file_name = "herp/derp";
  ASSERT_OK_AND_ASSIGN(
      auto fs, fs::internal::MockFileSystem::Make(fs::kNoTime, {fs::File(file_name)}));
  result = format.Inspect({file_name, fs.get()});
  EXPECT_RAISES_WITH_MESSAGE_THAT(Invalid, testing::HasSubstr(file_name),
                                  result.status());
}

TEST_F(TestIpcFileFormat, ScanRecordBatchReaderProjected) {
  schema_ = schema({field("f64", float64()), field("i64", int64()),
                    field("f32", float32()), field("i32", int32())});

  opts_ = ScanOptions::Make(schema_);
  opts_->projector = RecordBatchProjector(SchemaFromColumnNames(schema_, {"f64"}));
  opts_->filter = equal(field_ref("i32"), scalar(0));

  // NB: projector is applied by the scanner; IpcFragment does not evaluate it so
  // we will not drop "i32" even though it is not in the projector's schema
  auto expected_schema = schema({field("f64", float64()), field("i32", int32())});

  auto reader = GetRecordBatchReader();
  auto source = GetFileSource(reader.get());
  auto fragment = std::make_shared<IpcFragment>(*source, opts_);

  int64_t row_count = 0;

  for (auto maybe_batch : Batches(fragment.get())) {
    ASSERT_OK_AND_ASSIGN(auto batch, std::move(maybe_batch));
    row_count += batch->num_rows();
    AssertSchemaEqual(*batch->schema(), *expected_schema,
                      /*check_metadata=*/false);
  }

  ASSERT_EQ(row_count, kNumRows);
}

TEST_F(TestIpcFileFormat, ScanRecordBatchReaderProjectedMissingCols) {
  auto reader_without_i32 = GetRecordBatchReader(
      schema({field("f64", float64()), field("i64", int64()), field("f32", float32())}));

  auto reader_without_f64 = GetRecordBatchReader(
      schema({field("i64", int64()), field("f32", float32()), field("i32", int32())}));

  auto reader =
      GetRecordBatchReader(schema({field("f64", float64()), field("i64", int64()),
                                   field("f32", float32()), field("i32", int32())}));

  schema_ = reader->schema();
  opts_ = ScanOptions::Make(schema_);
  opts_->projector = RecordBatchProjector(SchemaFromColumnNames(schema_, {"f64"}));
  opts_->filter = equal(field_ref("i32"), scalar(0));

  for (auto reader : {reader.get(), reader_without_i32.get(), reader_without_f64.get()}) {
    auto source = GetFileSource(reader);
    auto fragment = std::make_shared<IpcFragment>(*source, opts_);

    // NB: projector is applied by the scanner; Fragment does not evaluate it.
    // We will not drop "i32" even though it is not in the projector's schema.
    //
    // in the case where a file doesn't contain a referenced field, we won't
    // materialize it (the filter/projector will populate it with nulls later)
    std::shared_ptr<Schema> expected_schema;
    if (reader == reader_without_i32.get()) {
      expected_schema = schema({field("f64", float64())});
    } else if (reader == reader_without_f64.get()) {
      expected_schema = schema({field("i32", int32())});
    } else {
      expected_schema = schema({field("f64", float64()), field("i32", int32())});
    }

    int64_t row_count = 0;

    for (auto maybe_batch : Batches(fragment.get())) {
      ASSERT_OK_AND_ASSIGN(auto batch, std::move(maybe_batch));
      row_count += batch->num_rows();
      AssertSchemaEqual(*batch->schema(), *expected_schema,
                        /*check_metadata=*/false);
    }

    ASSERT_EQ(row_count, kNumRows);
  }
}

TEST_F(TestIpcFileFormat, Inspect) {
  auto reader = GetRecordBatchReader();
  auto source = GetFileSource(reader.get());
  auto format = IpcFileFormat();

  ASSERT_OK_AND_ASSIGN(auto actual, format.Inspect(*source.get()));
  EXPECT_EQ(*actual, *schema_);
}

TEST_F(TestIpcFileFormat, IsSupported) {
  auto reader = GetRecordBatchReader();
  auto source = GetFileSource(reader.get());
  auto format = IpcFileFormat();

  bool supported = false;

  std::shared_ptr<Buffer> buf = std::make_shared<Buffer>(util::string_view(""));
  ASSERT_OK_AND_ASSIGN(supported, format.IsSupported(FileSource(buf)));
  ASSERT_EQ(supported, false);

  buf = std::make_shared<Buffer>(util::string_view("corrupted"));
  ASSERT_OK_AND_ASSIGN(supported, format.IsSupported(FileSource(buf)));
  ASSERT_EQ(supported, false);

  ASSERT_OK_AND_ASSIGN(supported, format.IsSupported(*source));
  EXPECT_EQ(supported, true);
}

}  // namespace dataset
}  // namespace arrow
