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

# cython: language_level = 3

from pyarrow.lib cimport check_status
from pyarrow.compat import frombytes, tobytes
from pyarrow.includes.common cimport *
from pyarrow.includes.libarrow cimport *
from pyarrow.includes.libarrow_fs cimport *
from pyarrow._fs cimport FileSystem


cdef class HadoopFileSystem(FileSystem):
    """
    HDFS backed FileSystem implementation

    Parameters
    ----------
    host : str
        HDFS host to connect to.
    port : int, default 8020
        HDFS port to connect to.
    replication : int, default 3
        Number of copies each block will have.
    buffer_size : int, default 0
        If 0, no buffering will happen otherwise the size of the temporary read
        and write buffer.
    default_block_size : int, default None
        None means the default configuration for HDFS, a typical block size is
        128 MB.
    """

    cdef:
        CHadoopFileSystem* hdfs

    def __init__(self, str host, int port=8020, str user=None,
                 int replication=3, int buffer_size=0,
                 default_block_size=None):
        cdef:
            CHdfsOptions options
            shared_ptr[CHadoopFileSystem] wrapped

        if not host.startswith(('hdfs://', 'viewfs://')):
            # TODO(kszucs): do more sanitization
            host = 'hdfs://{}'.format(host)

        options.ConfigureEndPoint(tobytes(host), int(port))
        options.ConfigureHdfsReplication(replication)
        options.ConfigureHdfsBufferSize(buffer_size)

        if user is not None:
            options.ConfigureHdfsUser(tobytes(user))
        if default_block_size is not None:
            options.ConfigureHdfsBlockSize(default_block_size)

        with nogil:
            wrapped = GetResultValue(CHadoopFileSystem.Make(options))
        self.init(<shared_ptr[CFileSystem]> wrapped)

    cdef init(self, const shared_ptr[CFileSystem]& wrapped):
        FileSystem.init(self, wrapped)
        self.hdfs = <CHadoopFileSystem*> wrapped.get()

    @staticmethod
    def from_uri(uri):
        """
        Instantiate HadoopFileSystem object from an URI string.

        The following two calls are equivalent
        * HadoopFileSystem.from_uri('hdfs://localhost:8020/?user=test'
                                    '&replication=1')
        * HadoopFileSystem('localhost', port=8020, user='test', replication=1)

        Parameters
        ----------
        uri : str
            A string URI describing the connection to HDFS.
            In order to change the user, replication, buffer_size or
            default_block_size pass the values as query parts.

        Returns
        -------
        HadoopFileSystem
        """
        cdef:
            HadoopFileSystem self = HadoopFileSystem.__new__(HadoopFileSystem)
            shared_ptr[CHadoopFileSystem] wrapped
            CHdfsOptions options

        options = GetResultValue(CHdfsOptions.FromUriString(tobytes(uri)))
        with nogil:
            wrapped = GetResultValue(CHadoopFileSystem.Make(options))

        self.init(<shared_ptr[CFileSystem]> wrapped)
        return self

    def __reduce__(self):
        cdef CHdfsOptions opts = self.hdfs.options()
        return (
            HadoopFileSystem, (
                frombytes(opts.connection_config.host),
                opts.connection_config.port,
                frombytes(opts.connection_config.user),
                opts.replication,
                opts.buffer_size,
                opts.default_block_size,
            )
        )
