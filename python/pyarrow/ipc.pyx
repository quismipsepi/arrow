# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# Cython wrappers for arrow::ipc

# cython: profile=False
# distutils: language = c++
# cython: embedsignature = True

from pyarrow.includes.libarrow cimport *
from pyarrow.includes.libarrow_io cimport *
from pyarrow.includes.libarrow_ipc cimport *
cimport pyarrow.includes.pyarrow as pyarrow

from pyarrow.error cimport check_cstatus
from pyarrow.io cimport NativeFile
from pyarrow.schema cimport Schema
from pyarrow.table cimport RecordBatch

from pyarrow.compat import frombytes, tobytes
import pyarrow.io as io

cimport cpython as cp


cdef get_reader(source, shared_ptr[ReadableFileInterface]* reader):
    cdef NativeFile nf

    if isinstance(source, bytes):
        source = io.BytesReader(source)
    elif not isinstance(source, io.NativeFile) and hasattr(source, 'read'):
        # Optimistically hope this is file-like
        source = io.PythonFileInterface(source, mode='r')

    if isinstance(source, NativeFile):
        nf = source

        # TODO: what about read-write sources (e.g. memory maps)
        if not nf.is_readonly:
            raise IOError('Native file is not readable')

        nf.read_handle(reader)
    else:
        raise TypeError('Unable to read from object of type: {0}'
                        .format(type(source)))


cdef get_writer(source, shared_ptr[OutputStream]* writer):
    cdef NativeFile nf

    if not isinstance(source, io.NativeFile) and hasattr(source, 'write'):
        # Optimistically hope this is file-like
        source = io.PythonFileInterface(source, mode='w')

    if isinstance(source, io.NativeFile):
        nf = source

        if nf.is_readonly:
            raise IOError('Native file is not writeable')

        nf.write_handle(writer)
    else:
        raise TypeError('Unable to read from object of type: {0}'
                        .format(type(source)))


cdef class ArrowFileWriter:
    cdef:
        shared_ptr[CFileWriter] writer
        shared_ptr[OutputStream] sink
        bint closed

    def __cinit__(self, sink, Schema schema):
        self.closed = True
        get_writer(sink, &self.sink)

        with nogil:
            check_cstatus(CFileWriter.Open(self.sink.get(), schema.sp_schema,
                                           &self.writer))

        self.closed = False

    def __dealloc__(self):
        if not self.closed:
            self.close()

    def write_record_batch(self, RecordBatch batch):
        cdef CRecordBatch* bptr = batch.batch
        with nogil:
            check_cstatus(self.writer.get()
                          .WriteRecordBatch(bptr.columns(), bptr.num_rows()))

    def close(self):
        with nogil:
            check_cstatus(self.writer.get().Close())
        self.closed = True


cdef class ArrowFileReader:
    cdef:
        shared_ptr[CFileReader] reader

    def __cinit__(self, source, footer_offset=None):
        cdef shared_ptr[ReadableFileInterface] reader
        get_reader(source, &reader)

        cdef int64_t offset = 0
        if footer_offset is not None:
            offset = footer_offset

        with nogil:
            if offset != 0:
                check_cstatus(CFileReader.Open2(reader, offset, &self.reader))
            else:
                check_cstatus(CFileReader.Open(reader, &self.reader))

    property num_dictionaries:

        def __get__(self):
            return self.reader.get().num_dictionaries()

    property num_record_batches:

        def __get__(self):
            return self.reader.get().num_record_batches()

    def get_record_batch(self, int i):
        cdef:
            shared_ptr[CRecordBatch] batch
            RecordBatch result

        if i < 0 or i >= self.num_record_batches:
            raise ValueError('Batch number {0} out of range'.format(i))

        with nogil:
            check_cstatus(self.reader.get().GetRecordBatch(i, &batch))

        result = RecordBatch()
        result.init(batch)

        return result
