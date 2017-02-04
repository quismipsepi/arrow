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

# cython: profile=False
# distutils: language = c++
# cython: embedsignature = True

from pyarrow.includes.libarrow cimport CMemoryPool
from pyarrow.includes.pyarrow cimport set_default_memory_pool, get_memory_pool

cdef class MemoryPool:
    cdef init(self, CMemoryPool* pool):
        self.pool = pool

    def bytes_allocated(self):
        return self.pool.bytes_allocated()

cdef CMemoryPool* maybe_unbox_memory_pool(MemoryPool memory_pool):
    if memory_pool is None:
        return get_memory_pool()
    else:
        return memory_pool.pool

def default_pool():
    cdef: 
        MemoryPool pool = MemoryPool()
    pool.init(get_memory_pool())
    return pool

def set_default_pool(MemoryPool pool):
    set_default_memory_pool(pool.pool)

def total_allocated_bytes():
    cdef CMemoryPool* pool = get_memory_pool()
    return pool.bytes_allocated()
