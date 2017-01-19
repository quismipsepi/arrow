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

from pyarrow.includes.common cimport *
from pyarrow.includes.libarrow cimport (CDataType, CDictionaryType,
                                        CField, CSchema)

cdef class DataType:
    cdef:
        shared_ptr[CDataType] sp_type
        CDataType* type

    cdef init(self, const shared_ptr[CDataType]& type)


cdef class DictionaryType(DataType):
    cdef:
        const CDictionaryType* dict_type

cdef class Field:
    cdef:
        shared_ptr[CField] sp_field
        CField* field

    cdef readonly:
        DataType type

    cdef init(self, const shared_ptr[CField]& field)

cdef class Schema:
    cdef:
        shared_ptr[CSchema] sp_schema
        CSchema* schema

    cdef init(self, const vector[shared_ptr[CField]]& fields)
    cdef init_schema(self, const shared_ptr[CSchema]& schema)

cdef DataType box_data_type(const shared_ptr[CDataType]& type)
cdef Field box_field(const shared_ptr[CField]& field)
cdef Schema box_schema(const shared_ptr[CSchema]& schema)
