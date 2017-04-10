<!---
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License. See accompanying LICENSE file.
-->

# Interprocess messaging / communication (IPC)

## Encapsulated message format

Data components in the stream and file formats are represented as encapsulated
*messages* consisting of:

* A length prefix indicating the metadata size
* The message metadata as a [Flatbuffer][3]
* Padding bytes to an 8-byte boundary
* The message body

Schematically, we have:

```
<metadata_size: int32>
<metadata_flatbuffer: bytes>
<padding>
<message body>
```

The `metadata_size` includes the size of the flatbuffer plus padding. The
`Message` flatbuffer includes a version number, the particular message (as a
flatbuffer union), and the size of the message body:

```
table Message {
  version: org.apache.arrow.flatbuf.MetadataVersion;
  header: MessageHeader;
  bodyLength: long;
}
```

Currently, we support 4 types of messages:

* Schema
* RecordBatch
* DictionaryBatch
* Tensor

## Streaming format

We provide a streaming format for record batches. It is presented as a sequence
of encapsulated messages, each of which follows the format above. The schema
comes first in the stream, and it is the same for all of the record batches
that follow. If any fields in the schema are dictionary-encoded, one or more
`DictionaryBatch` messages will follow the schema.

```
<SCHEMA>
<DICTIONARY 0>
...
<DICTIONARY k - 1>
<RECORD BATCH 0>
...
<RECORD BATCH n - 1>
<EOS [optional]: int32>
```

When a stream reader implementation is reading a stream, after each message, it
may read the next 4 bytes to know how large the message metadata that follows
is. Once the message flatbuffer is read, you can then read the message body.

The stream writer can signal end-of-stream (EOS) either by writing a 0 length
as an `int32` or simply closing the stream interface.

## File format

We define a "file format" supporting random access in a very similar format to
the streaming format. The file starts and ends with a magic string `ARROW1`
(plus padding). What follows in the file is identical to the stream format. At
the end of the file, we write a *footer* including offsets and sizes for each
of the data blocks in the file, so that random access is possible. See
[format/File.fbs][1] for the precise details of the file footer.

Schematically we have:

```
<magic number "ARROW1">
<empty padding bytes [to 8 byte boundary]>
<STREAMING FORMAT>
<FOOTER>
<FOOTER SIZE: int32>
<magic number "ARROW1">
```

### RecordBatch body structure

The `RecordBatch` metadata contains a depth-first (pre-order) flattened set of
field metadata and physical memory buffers (some comments from [Message.fbs][2]
have been shortened / removed):

```
table RecordBatch {
  length: long;
  nodes: [FieldNode];
  buffers: [Buffer];
}

struct FieldNode {
  length: long;
  null_count: long;
}

struct Buffer {
  /// The shared memory page id where this buffer is located. Currently this is
  /// not used
  page: int;

  /// The relative offset into the shared memory page where the bytes for this
  /// buffer starts
  offset: long;

  /// The absolute length (in bytes) of the memory buffer. The memory is found
  /// from offset (inclusive) to offset + length (non-inclusive).
  length: long;
}
```

In the context of a file, the `page` is not used, and the `Buffer` offsets use
as a frame of reference the start of the message body. So, while in a general
IPC setting these offsets may be anyplace in one or more shared memory regions,
in the file format the offsets start from 0.

The location of a record batch and the size of the metadata block as well as
the body of buffers is stored in the file footer:

```
struct Block {
  offset: long;
  metaDataLength: int;
  bodyLength: long;
}
```

Some notes about this

* The `Block` offset indicates the starting byte of the record batch.
* The metadata length includes the flatbuffer size, the record batch metadata
  flatbuffer, and any padding bytes

### Dictionary Batches

Dictionary batches have not yet been implemented, while they are provided for
in the metadata. For the time being, the `DICTIONARY` segments shown above in
the file do not appear in any of the file implementations.

### Tensor (Multi-dimensional Array) Message Format

The `Tensor` message types provides a way to write a multidimensional array of
fixed-size values (such as a NumPy ndarray) using Arrow's shared memory
tools. Arrow implementations in general are not required to implement this data
format, though we provide a reference implementation in C++.

When writing a standalone encapsulated tensor message, we use the format as
indicated above, but additionally align the starting offset (if writing to a
shared memory region) to be a multiple of 8:

```
<PADDING>
<metadata size: int32>
<metadata>
<tensor body>
```

[1]: https://github.com/apache/arrow/blob/master/format/File.fbs
[2]: https://github.com/apache/arrow/blob/master/format/Message.fbs
[3]: https://github.com/google]/flatbuffers
