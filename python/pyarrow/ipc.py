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

# Arrow file and stream reader/writer classes, and other messaging tools

import pyarrow.io as io


class StreamReader(io._StreamReader):
    """
    Reader for the Arrow streaming binary format

    Parameters
    ----------
    source : str, pyarrow.NativeFile, or file-like Python object
        Either a file path, or a readable file object
    """
    def __init__(self, source):
        self._open(source)

    def __iter__(self):
        while True:
            yield self.get_next_batch()


class StreamWriter(io._StreamWriter):
    """
    Writer for the Arrow streaming binary format

    Parameters
    ----------
    sink : str, pyarrow.NativeFile, or file-like Python object
        Either a file path, or a writeable file object
    schema : pyarrow.Schema
        The Arrow schema for data to be written to the file
    """
    def __init__(self, sink, schema):
        self._open(sink, schema)


class FileReader(io._FileReader):
    """
    Class for reading Arrow record batch data from the Arrow binary file format

    Parameters
    ----------
    source : str, pyarrow.NativeFile, or file-like Python object
        Either a file path, or a readable file object
    footer_offset : int, default None
        If the file is embedded in some larger file, this is the byte offset to
        the very end of the file data
    """
    def __init__(self, source, footer_offset=None):
        self._open(source, footer_offset=footer_offset)


class FileWriter(io._FileWriter):
    """
    Writer to create the Arrow binary file format

    Parameters
    ----------
    sink : str, pyarrow.NativeFile, or file-like Python object
        Either a file path, or a writeable file object
    schema : pyarrow.Schema
        The Arrow schema for data to be written to the file
    """
    def __init__(self, sink, schema):
        self._open(sink, schema)
