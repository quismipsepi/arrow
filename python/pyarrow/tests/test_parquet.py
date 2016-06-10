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

from pyarrow.compat import unittest
import pyarrow as arrow
import pyarrow.parquet

A = arrow

import numpy as np
import os.path
import pandas as pd

import pandas.util.testing as pdt


def test_single_pylist_column_roundtrip(tmpdir):
    for dtype in [int, float]:
        filename = tmpdir.join('single_{}_column.parquet'.format(dtype.__name__))
        data = [A.from_pylist(list(map(dtype, range(5))))]
        table = A.Table.from_arrays(('a', 'b'), data, 'table_name')
        A.parquet.write_table(table, filename.strpath)
        table_read = pyarrow.parquet.read_table(filename.strpath)
        for col_written, col_read in zip(table.itercolumns(), table_read.itercolumns()):
            assert col_written.name == col_read.name
            assert col_read.data.num_chunks == 1
            data_written = col_written.data.chunk(0)
            data_read = col_read.data.chunk(0)
            assert data_written.equals(data_read)

def test_pandas_rountrip(tmpdir):
    size = 10000
    df = pd.DataFrame({
        'int32': np.arange(size, dtype=np.int32),
        'int64': np.arange(size, dtype=np.int64),
        'float32': np.arange(size, dtype=np.float32),
        'float64': np.arange(size, dtype=np.float64)
    })
    filename = tmpdir.join('pandas_rountrip.parquet')
    arrow_table = A.from_pandas_dataframe(df)
    A.parquet.write_table(arrow_table, filename.strpath)
    table_read = pyarrow.parquet.read_table(filename.strpath)
    df_read = table_read.to_pandas()
    pdt.assert_frame_equal(df, df_read)

