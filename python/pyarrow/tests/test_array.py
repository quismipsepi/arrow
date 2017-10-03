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

import datetime
import pytest
import sys

import numpy as np
import pandas as pd
import pandas.util.testing as tm

import pyarrow as pa
from pyarrow.pandas_compat import get_logical_type
import pyarrow.formatting as fmt


def test_total_bytes_allocated():
    assert pa.total_allocated_bytes() == 0


def test_repr_on_pre_init_array():
    arr = pa.Array()
    assert len(repr(arr)) > 0


def test_getitem_NA():
    arr = pa.array([1, None, 2])
    assert arr[1] is pa.NA


def test_list_format():
    arr = pa.array([[1], None, [2, 3, None]])
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
    arr = pa.array(['', None, 'foo'])
    result = fmt.array_format(arr)
    expected = """\
[
  '',
  NA,
  'foo'
]"""
    assert result == expected


def test_long_array_format():
    arr = pa.array(range(100))
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

    arr = pa.array(range(10))

    for i in range(10):
        np_arr = arr.to_pandas()
        assert sys.getrefcount(np_arr) == 2
        np_arr = None  # noqa

    assert sys.getrefcount(arr) == 2

    for i in range(10):
        arr = pa.array(range(10))
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
    arr = pa.array(range(10))

    sliced = arr.slice(2)
    expected = pa.array(range(2, 10))
    assert sliced.equals(expected)

    sliced2 = arr.slice(2, 4)
    expected2 = pa.array(range(2, 6))
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


def test_array_factory_invalid_type():
    arr = np.array([datetime.timedelta(1), datetime.timedelta(2)])
    with pytest.raises(ValueError):
        pa.array(arr)


def test_array_ref_to_ndarray_base():
    arr = np.array([1, 2, 3])

    refcount = sys.getrefcount(arr)
    arr2 = pa.array(arr)  # noqa
    assert sys.getrefcount(arr) == (refcount + 1)


def test_dictionary_from_numpy():
    indices = np.repeat([0, 1, 2], 2)
    dictionary = np.array(['foo', 'bar', 'baz'], dtype=object)
    mask = np.array([False, False, True, False, False, False])

    d1 = pa.DictionaryArray.from_arrays(indices, dictionary)
    d2 = pa.DictionaryArray.from_arrays(indices, dictionary, mask=mask)

    for i in range(len(indices)):
        assert d1[i].as_py() == dictionary[indices[i]]

        if mask[i]:
            assert d2[i] is pa.NA
        else:
            assert d2[i].as_py() == dictionary[indices[i]]


def test_dictionary_from_boxed_arrays():
    indices = np.repeat([0, 1, 2], 2)
    dictionary = np.array(['foo', 'bar', 'baz'], dtype=object)

    iarr = pa.array(indices)
    darr = pa.array(dictionary)

    d1 = pa.DictionaryArray.from_arrays(iarr, darr)

    for i in range(len(indices)):
        assert d1[i].as_py() == dictionary[indices[i]]


def test_dictionary_with_pandas():
    indices = np.repeat([0, 1, 2], 2)
    dictionary = np.array(['foo', 'bar', 'baz'], dtype=object)
    mask = np.array([False, False, True, False, False, False])

    d1 = pa.DictionaryArray.from_arrays(indices, dictionary)
    d2 = pa.DictionaryArray.from_arrays(indices, dictionary, mask=mask)

    pandas1 = d1.to_pandas()
    ex_pandas1 = pd.Categorical.from_codes(indices, categories=dictionary)

    tm.assert_series_equal(pd.Series(pandas1), pd.Series(ex_pandas1))

    pandas2 = d2.to_pandas()
    ex_pandas2 = pd.Categorical.from_codes(np.where(mask, -1, indices),
                                           categories=dictionary)

    tm.assert_series_equal(pd.Series(pandas2), pd.Series(ex_pandas2))


def test_list_from_arrays():
    offsets_arr = np.array([0, 2, 5, 8], dtype='i4')
    offsets = pa.array(offsets_arr, type='int32')
    pyvalues = [b'a', b'b', b'c', b'd', b'e', b'f', b'g', b'h']
    values = pa.array(pyvalues, type='binary')

    result = pa.ListArray.from_arrays(offsets, values)
    expected = pa.array([pyvalues[:2], pyvalues[2:5], pyvalues[5:8]])

    assert result.equals(expected)


def _check_cast_case(case, safe=True):
    in_data, in_type, out_data, out_type = case

    in_arr = pa.array(in_data, type=in_type)

    casted = in_arr.cast(out_type, safe=safe)
    expected = pa.array(out_data, type=out_type)
    assert casted.equals(expected)


def test_cast_integers_safe():
    safe_cases = [
        (np.array([0, 1, 2, 3], dtype='i1'), 'int8',
         np.array([0, 1, 2, 3], dtype='i4'), pa.int32()),
        (np.array([0, 1, 2, 3], dtype='i1'), 'int8',
         np.array([0, 1, 2, 3], dtype='u4'), pa.uint16()),
        (np.array([0, 1, 2, 3], dtype='i1'), 'int8',
         np.array([0, 1, 2, 3], dtype='u1'), pa.uint8()),
        (np.array([0, 1, 2, 3], dtype='i1'), 'int8',
         np.array([0, 1, 2, 3], dtype='f8'), pa.float64())
    ]

    for case in safe_cases:
        _check_cast_case(case)

    unsafe_cases = [
        (np.array([50000], dtype='i4'), 'int32', 'int16'),
        (np.array([70000], dtype='i4'), 'int32', 'uint16'),
        (np.array([-1], dtype='i4'), 'int32', 'uint16'),
        (np.array([50000], dtype='u2'), 'uint16', 'int16')
    ]
    for in_data, in_type, out_type in unsafe_cases:
        in_arr = pa.array(in_data, type=in_type)

        with pytest.raises(pa.ArrowInvalid):
            in_arr.cast(out_type)


def test_cast_integers_unsafe():
    # We let NumPy do the unsafe casting
    unsafe_cases = [
        (np.array([50000], dtype='i4'), 'int32',
         np.array([50000], dtype='i2'), pa.int16()),
        (np.array([70000], dtype='i4'), 'int32',
         np.array([70000], dtype='u2'), pa.uint16()),
        (np.array([-1], dtype='i4'), 'int32',
         np.array([-1], dtype='u2'), pa.uint16()),
        (np.array([50000], dtype='u2'), pa.uint16(),
         np.array([50000], dtype='i2'), pa.int16())
    ]

    for case in unsafe_cases:
        _check_cast_case(case, safe=False)


def test_cast_signed_to_unsigned():
    safe_cases = [
        (np.array([0, 1, 2, 3], dtype='i1'), pa.uint8(),
         np.array([0, 1, 2, 3], dtype='u1'), pa.uint8()),
        (np.array([0, 1, 2, 3], dtype='i2'), pa.uint16(),
         np.array([0, 1, 2, 3], dtype='u2'), pa.uint16())
    ]

    for case in safe_cases:
        _check_cast_case(case)


def test_simple_type_construction():
    result = pa.lib.TimestampType()
    with pytest.raises(TypeError):
        str(result)


@pytest.mark.parametrize(
    ('type', 'expected'),
    [
        (pa.null(), 'float64'),
        (pa.bool_(), 'bool'),
        (pa.int8(), 'int8'),
        (pa.int16(), 'int16'),
        (pa.int32(), 'int32'),
        (pa.int64(), 'int64'),
        (pa.uint8(), 'uint8'),
        (pa.uint16(), 'uint16'),
        (pa.uint32(), 'uint32'),
        (pa.uint64(), 'uint64'),
        (pa.float16(), 'float16'),
        (pa.float32(), 'float32'),
        (pa.float64(), 'float64'),
        (pa.date32(), 'date'),
        (pa.date64(), 'date'),
        (pa.binary(), 'bytes'),
        (pa.binary(length=4), 'bytes'),
        (pa.string(), 'unicode'),
        (pa.list_(pa.list_(pa.int16())), 'list[list[int16]]'),
        (pa.decimal(18, 3), 'decimal'),
        (pa.timestamp('ms'), 'datetime'),
        (pa.timestamp('us', 'UTC'), 'datetimetz'),
        (pa.time32('s'), 'time'),
        (pa.time64('us'), 'time')
    ]
)
def test_logical_type(type, expected):
    assert get_logical_type(type) == expected


def test_array_conversions_no_sentinel_values():
    arr = np.array([1, 2, 3, 4], dtype='int8')
    refcount = sys.getrefcount(arr)
    arr2 = pa.array(arr)  # noqa
    assert sys.getrefcount(arr) == (refcount + 1)

    assert arr2.type == 'int8'

    arr3 = pa.array(np.array([1, np.nan, 2, 3, np.nan, 4], dtype='float32'),
                    type='float32')
    assert arr3.type == 'float32'
    assert arr3.null_count == 0
