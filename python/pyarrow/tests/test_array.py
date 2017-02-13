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

import sys

import pytest

import pyarrow
import pyarrow.formatting as fmt


def test_total_bytes_allocated():
    assert pyarrow.total_allocated_bytes() == 0


def test_repr_on_pre_init_array():
    arr = pyarrow.array.Array()
    assert len(repr(arr)) > 0


def test_getitem_NA():
    arr = pyarrow.from_pylist([1, None, 2])
    assert arr[1] is pyarrow.NA


def test_list_format():
    arr = pyarrow.from_pylist([[1], None, [2, 3, None]])
    result = fmt.array_format(arr)
    expected = """\
[
  [1],
  NA,
  [2,
   3,
   NA]
]"""
    assert result == expected


def test_string_format():
    arr = pyarrow.from_pylist(['', None, 'foo'])
    result = fmt.array_format(arr)
    expected = """\
[
  '',
  NA,
  'foo'
]"""
    assert result == expected


def test_long_array_format():
    arr = pyarrow.from_pylist(range(100))
    result = fmt.array_format(arr, window=2)
    expected = """\
[
  0,
  1,
  ...
  98,
  99
]"""
    assert result == expected


def test_to_pandas_zero_copy():
    import gc

    arr = pyarrow.from_pylist(range(10))

    for i in range(10):
        np_arr = arr.to_pandas()
        assert sys.getrefcount(np_arr) == 2
        np_arr = None  # noqa

    assert sys.getrefcount(arr) == 2

    for i in range(10):
        arr = pyarrow.from_pylist(range(10))
        np_arr = arr.to_pandas()
        arr = None
        gc.collect()

        # Ensure base is still valid

        # Because of py.test's assert inspection magic, if you put getrefcount
        # on the line being examined, it will be 1 higher than you expect
        base_refcount = sys.getrefcount(np_arr.base)
        assert base_refcount == 2
        np_arr.sum()


def test_array_slice():
    arr = pyarrow.from_pylist(range(10))

    sliced = arr.slice(2)
    expected = pyarrow.from_pylist(range(2, 10))
    assert sliced.equals(expected)

    sliced2 = arr.slice(2, 4)
    expected2 = pyarrow.from_pylist(range(2, 6))
    assert sliced2.equals(expected2)

    # 0 offset
    assert arr.slice(0).equals(arr)

    # Slice past end of array
    assert len(arr.slice(len(arr))) == 0

    with pytest.raises(IndexError):
        arr.slice(-1)

    # Test slice notation
    assert arr[2:].equals(arr.slice(2))

    assert arr[2:5].equals(arr.slice(2, 3))

    assert arr[-5:].equals(arr.slice(len(arr) - 5))

    with pytest.raises(IndexError):
        arr[::-1]

    with pytest.raises(IndexError):
        arr[::2]
