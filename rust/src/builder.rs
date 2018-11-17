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

//! Defines a `BufferBuilder` capable of creating a `Buffer` which can be used as an internal
//! buffer in an `ArrayData` object.

use std::any::Any;
use std::io::Write;
use std::marker::PhantomData;
use std::mem;

use array::{Array, ListArray, PrimitiveArray};
use array_data::ArrayData;
use buffer::{Buffer, MutableBuffer};
use datatypes::{ArrowPrimitiveType, DataType, ToByteSlice};
use error::{ArrowError, Result};
use util::bit_util;

/// Buffer builder with zero-copy build method
pub struct BufferBuilder<T>
where
    T: ArrowPrimitiveType,
{
    buffer: MutableBuffer,
    len: i64,
    _marker: PhantomData<T>,
}

macro_rules! impl_buffer_builder {
    ($native_ty:ident) => {
        impl BufferBuilder<$native_ty> {
            /// Creates a builder with a fixed initial capacity
            pub fn new(capacity: i64) -> Self {
                let buffer = MutableBuffer::new(capacity as usize * mem::size_of::<$native_ty>());
                Self {
                    buffer,
                    len: 0,
                    _marker: PhantomData,
                }
            }

            /// Returns the number of array elements (slots) in the builder
            pub fn len(&self) -> i64 {
                self.len
            }

            // Advances the `len` of the underlying `Buffer` by `i` slots of type T
            fn advance(&mut self, i: i64) -> Result<()> {
                let new_buffer_len = (self.len + i) as usize * mem::size_of::<$native_ty>();
                self.buffer.resize(new_buffer_len)?;
                self.len += i;
                Ok(())
            }

            /// Returns the current capacity of the builder (number of elements)
            pub fn capacity(&self) -> i64 {
                let byte_capacity = self.buffer.capacity();
                (byte_capacity / mem::size_of::<$native_ty>()) as i64
            }

            /// Pushes a value into the builder, growing the internal buffer as needed.
            pub fn push(&mut self, v: $native_ty) -> Result<()> {
                self.reserve(1)?;
                self.write_bytes(v.to_byte_slice(), 1)
            }

            /// Pushes a slice of type `T`, growing the internal buffer as needed.
            pub fn push_slice(&mut self, slice: &[$native_ty]) -> Result<()> {
                let array_slots = slice.len() as i64;
                self.reserve(array_slots)?;
                self.write_bytes(slice.to_byte_slice(), array_slots)
            }

            /// Reserves memory for `n` elements of type `T`.
            pub fn reserve(&mut self, n: i64) -> Result<()> {
                let new_capacity = self.len + n;
                let byte_capacity = mem::size_of::<$native_ty>() * new_capacity as usize;
                self.buffer.reserve(byte_capacity)?;
                Ok(())
            }

            /// Consumes this builder and returns an immutable `Buffer`.
            pub fn finish(self) -> Buffer {
                self.buffer.freeze()
            }

            /// Writes a byte slice to the underlying buffer and updates the `len`, i.e. the number array
            /// elements in the builder.  Also, converts the `io::Result` required by the `Write` trait
            /// to the Arrow `Result` type.
            fn write_bytes(&mut self, bytes: &[u8], len_added: i64) -> Result<()> {
                let write_result = self.buffer.write(bytes);
                // `io::Result` has many options one of which we use, so pattern matching is overkill here
                if write_result.is_err() {
                    Err(ArrowError::MemoryError(
                        "Could not write to Buffer, not big enough".to_string(),
                    ))
                } else {
                    self.len += len_added;
                    Ok(())
                }
            }
        }
    };
}

impl_buffer_builder!(u8);
impl_buffer_builder!(u16);
impl_buffer_builder!(u32);
impl_buffer_builder!(u64);
impl_buffer_builder!(i8);
impl_buffer_builder!(i16);
impl_buffer_builder!(i32);
impl_buffer_builder!(i64);
impl_buffer_builder!(f32);
impl_buffer_builder!(f64);

impl BufferBuilder<bool> {
    /// Creates a builder with a fixed initial capacity.
    pub fn new(capacity: i64) -> Self {
        let byte_capacity = bit_util::ceil(capacity, 8);
        let actual_capacity = bit_util::round_upto_multiple_of_64(byte_capacity) as usize;
        let mut buffer = MutableBuffer::new(actual_capacity);
        buffer.set_null_bits(0, actual_capacity);
        Self {
            buffer,
            len: 0,
            _marker: PhantomData,
        }
    }

    /// Returns the number of array elements (slots) in the builder.
    pub fn len(&self) -> i64 {
        self.len
    }

    // Advances the `len` of the underlying `Buffer` by `i` slots of type T
    pub fn advance(&mut self, i: i64) -> Result<()> {
        let new_buffer_len = bit_util::ceil(self.len + i, 8);
        self.buffer.resize(new_buffer_len as usize)?;
        self.len += i;
        Ok(())
    }

    /// Returns the current capacity of the builder (number of elements)
    pub fn capacity(&self) -> i64 {
        let byte_capacity = self.buffer.capacity() as i64;
        byte_capacity * 8
    }

    /// Pushes a value into the builder, growing the internal buffer as needed.
    pub fn push(&mut self, v: bool) -> Result<()> {
        self.reserve(1)?;
        if v {
            // For performance the `len` of the buffer is not updated on each push but
            // is updated in the `freeze` method instead.
            unsafe {
                bit_util::set_bit_raw(self.buffer.raw_data() as *mut u8, (self.len) as usize);
            }
        }
        self.len += 1;
        Ok(())
    }

    /// Pushes a slice of type `T`, growing the internal buffer as needed.
    pub fn push_slice(&mut self, slice: &[bool]) -> Result<()> {
        let array_slots = slice.len();
        for i in 0..array_slots {
            self.push(slice[i])?;
        }
        Ok(())
    }

    /// Reserves memory for `n` elements of type `T`.
    pub fn reserve(&mut self, n: i64) -> Result<()> {
        let new_capacity = self.len + n;
        if new_capacity > self.capacity() {
            let new_byte_capacity = bit_util::ceil(new_capacity, 8) as usize;
            let existing_capacity = self.buffer.capacity();
            let new_capacity = self.buffer.reserve(new_byte_capacity)?;
            self.buffer
                .set_null_bits(existing_capacity, new_capacity - existing_capacity);
        }
        Ok(())
    }

    /// Consumes this and returns an immutable `Buffer`.
    pub fn finish(mut self) -> Buffer {
        // `push` does not update the buffer's `len` so do it before `freeze` is called.
        let new_buffer_len = bit_util::ceil(self.len, 8) as usize;
        debug_assert!(new_buffer_len >= self.buffer.len());
        self.buffer.resize(new_buffer_len).unwrap();
        self.buffer.freeze()
    }
}

/// Trait for dealing with different array builders at runtime
pub trait ArrayBuilder {
    /// The type of array that this builder creates
    type ArrayType;

    /// Returns the builder as an owned `Any` type so that it can be `downcast` to a specific
    /// implementation before calling it's `finish` method
    fn into_any(self) -> Box<Any>;

    /// Returns the number of array slots in the builder
    fn len(&self) -> i64;

    /// Builds the array
    fn finish(self) -> Self::ArrayType;
}

///  Array builder for fixed-width primitive types
pub struct PrimitiveArrayBuilder<T>
where
    T: ArrowPrimitiveType,
{
    values_builder: BufferBuilder<T>,
    bitmap_builder: BufferBuilder<bool>,
}

macro_rules! impl_primitive_array_builder {
    ($data_ty:path, $native_ty:ident) => {
        impl ArrayBuilder for PrimitiveArrayBuilder<$native_ty> {
            type ArrayType = PrimitiveArray<$native_ty>;

            /// Returns the builder as an owned `Any` type so that it can be `downcast` to a specific
            /// implementation before calling it's `finish` method
            fn into_any(self) -> Box<Any> {
                Box::new(self)
            }

            /// Returns the number of array slots in the builder
            fn len(&self) -> i64 {
                self.values_builder.len
            }

            /// Builds the PrimitiveArray
            fn finish(self) -> PrimitiveArray<$native_ty> {
                let len = self.len();
                let null_bit_buffer = self.bitmap_builder.finish();
                let data = ArrayData::builder($data_ty)
                    .len(len)
                    .null_count(len - bit_util::count_set_bits(null_bit_buffer.data()))
                    .add_buffer(self.values_builder.finish())
                    .null_bit_buffer(null_bit_buffer)
                    .build();
                PrimitiveArray::<$native_ty>::from(data)
            }
        }

        impl PrimitiveArrayBuilder<$native_ty> {
            /// Creates a new primitive array builder
            pub fn new(capacity: i64) -> Self {
                Self {
                    values_builder: BufferBuilder::<$native_ty>::new(capacity),
                    bitmap_builder: BufferBuilder::<bool>::new(capacity),
                }
            }

            /// Returns the capacity of this builder measured in slots of type `T`
            pub fn capacity(&self) -> i64 {
                self.values_builder.capacity()
            }

            /// Pushes a value of type `T` into the builder
            pub fn push(&mut self, v: $native_ty) -> Result<()> {
                self.bitmap_builder.push(true)?;
                self.values_builder.push(v)?;
                Ok(())
            }

            /// Pushes a null slot into the builder
            pub fn push_null(&mut self) -> Result<()> {
                self.bitmap_builder.push(false)?;
                self.values_builder.advance(1)?;
                Ok(())
            }

            /// Pushes an `Option<T>` into the builder
            pub fn push_option(&mut self, v: Option<$native_ty>) -> Result<()> {
                match v {
                    None => self.push_null()?,
                    Some(v) => self.push(v)?,
                };
                Ok(())
            }

            /// Pushes a slice of type `T` into the builder
            pub fn push_slice(&mut self, v: &[$native_ty]) -> Result<()> {
                self.bitmap_builder.push_slice(&vec![true; v.len()][..])?;
                self.values_builder.push_slice(v)?;
                Ok(())
            }
        }
    };
}

impl_primitive_array_builder!(DataType::Boolean, bool);
impl_primitive_array_builder!(DataType::UInt8, u8);
impl_primitive_array_builder!(DataType::UInt16, u16);
impl_primitive_array_builder!(DataType::UInt32, u32);
impl_primitive_array_builder!(DataType::UInt64, u64);
impl_primitive_array_builder!(DataType::Int8, i8);
impl_primitive_array_builder!(DataType::Int16, i16);
impl_primitive_array_builder!(DataType::Int32, i32);
impl_primitive_array_builder!(DataType::Int64, i64);
impl_primitive_array_builder!(DataType::Float32, f32);
impl_primitive_array_builder!(DataType::Float64, f64);

///  Array builder for `ListArray`
pub struct ListArrayBuilder<T>
where
    T: ArrayBuilder,
{
    offsets_builder: BufferBuilder<i32>,
    bitmap_builder: BufferBuilder<bool>,
    values_builder: T,
    len: i64,
}

impl<T: ArrayBuilder> ListArrayBuilder<T> {
    /// Creates a new `ListArrayBuilder` from a given values array builder
    pub fn new(values_builder: T) -> Self {
        let mut offsets_builder = BufferBuilder::<i32>::new(values_builder.len() + 1);
        offsets_builder.push(0).unwrap();
        Self {
            offsets_builder,
            bitmap_builder: BufferBuilder::<bool>::new(values_builder.len()),
            values_builder,
            len: 0,
        }
    }
}

macro_rules! impl_list_array_builder {
    ($builder_ty:ty) => {
        impl ArrayBuilder for ListArrayBuilder<$builder_ty> {
            type ArrayType = ListArray;

            /// Returns the builder as an owned `Any` type so that it can be `downcast` to a specific
            /// implementation before calling it's `finish` method.
            fn into_any(self) -> Box<Any> {
                Box::new(self)
            }

            /// Returns the number of array slots in the builder
            fn len(&self) -> i64 {
                self.len
            }

            /// Builds the `ListArray`
            fn finish(self) -> ListArray {
                let len = self.len();
                let values_arr = self
                    .values_builder
                    .into_any()
                    .downcast::<$builder_ty>()
                    .unwrap()
                    .finish();
                let values_data = values_arr.data();

                let null_bit_buffer = self.bitmap_builder.finish();
                let data =
                    ArrayData::builder(DataType::List(Box::new(values_data.data_type().clone())))
                        .len(len)
                        .null_count(len - bit_util::count_set_bits(null_bit_buffer.data()))
                        .add_buffer(self.offsets_builder.finish())
                        .add_child_data(values_data)
                        .null_bit_buffer(null_bit_buffer)
                        .build();

                ListArray::from(data)
            }
        }

        impl ListArrayBuilder<$builder_ty> {
            /// Returns the child array builder as a mutable reference.
            ///
            /// This mutable reference can be used to push values into the child array builder,
            /// but you must call `append` to delimit each distinct list value.
            pub fn values(&mut self) -> &mut $builder_ty {
                &mut self.values_builder
            }

            /// Finish the current variable-length list array slot
            pub fn append(&mut self, is_valid: bool) -> Result<()> {
                self.offsets_builder
                    .push(self.values_builder.len() as i32)?;
                self.bitmap_builder.push(is_valid)?;
                self.len += 1;
                Ok(())
            }
        }
    };
}

impl_list_array_builder!(PrimitiveArrayBuilder<bool>);
impl_list_array_builder!(PrimitiveArrayBuilder<u8>);
impl_list_array_builder!(PrimitiveArrayBuilder<u16>);
impl_list_array_builder!(PrimitiveArrayBuilder<u32>);
impl_list_array_builder!(PrimitiveArrayBuilder<u64>);
impl_list_array_builder!(PrimitiveArrayBuilder<i8>);
impl_list_array_builder!(PrimitiveArrayBuilder<i16>);
impl_list_array_builder!(PrimitiveArrayBuilder<i32>);
impl_list_array_builder!(PrimitiveArrayBuilder<i64>);
impl_list_array_builder!(PrimitiveArrayBuilder<f32>);
impl_list_array_builder!(PrimitiveArrayBuilder<f64>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<bool>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<u8>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<u16>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<u32>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<u64>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<i8>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<i16>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<i32>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<i64>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<f32>>);
impl_list_array_builder!(ListArrayBuilder<PrimitiveArrayBuilder<f64>>);

#[cfg(test)]
mod tests {

    use array::Array;

    use super::*;

    #[test]
    fn test_builder_i32_empty() {
        let b = BufferBuilder::<i32>::new(5);
        assert_eq!(0, b.len());
        assert_eq!(16, b.capacity());
        let a = b.finish();
        assert_eq!(0, a.len());
    }

    #[test]
    fn test_builder_i32_alloc_zero_bytes() {
        let mut b = BufferBuilder::<i32>::new(0);
        b.push(123).unwrap();
        let a = b.finish();
        assert_eq!(4, a.len());
    }

    #[test]
    fn test_builder_i32() {
        let mut b = BufferBuilder::<i32>::new(5);
        for i in 0..5 {
            b.push(i).unwrap();
        }
        assert_eq!(16, b.capacity());
        let a = b.finish();
        assert_eq!(20, a.len());
    }

    #[test]
    fn test_builder_i32_grow_buffer() {
        let mut b = BufferBuilder::<i32>::new(2);
        assert_eq!(16, b.capacity());
        for i in 0..20 {
            b.push(i).unwrap();
        }
        assert_eq!(32, b.capacity());
        let a = b.finish();
        assert_eq!(80, a.len());
    }

    #[test]
    fn test_reserve() {
        let mut b = BufferBuilder::<u8>::new(2);
        assert_eq!(64, b.capacity());
        b.reserve(64).unwrap();
        assert_eq!(64, b.capacity());
        b.reserve(65).unwrap();
        assert_eq!(128, b.capacity());

        let mut b = BufferBuilder::<i32>::new(2);
        assert_eq!(16, b.capacity());
        b.reserve(16).unwrap();
        assert_eq!(16, b.capacity());
        b.reserve(17).unwrap();
        assert_eq!(32, b.capacity());
    }

    #[test]
    fn test_push_slice() {
        let mut b = BufferBuilder::<u8>::new(0);
        b.push_slice("Hello, ".as_bytes()).unwrap();
        b.push_slice("World!".as_bytes()).unwrap();
        let buffer = b.finish();
        assert_eq!(13, buffer.len());

        let mut b = BufferBuilder::<i32>::new(0);
        b.push_slice(&[32, 54]).unwrap();
        let buffer = b.finish();
        assert_eq!(8, buffer.len());
    }

    #[test]
    fn test_write_bytes() {
        let mut b = BufferBuilder::<bool>::new(4);
        b.push(false).unwrap();
        b.push(true).unwrap();
        b.push(false).unwrap();
        b.push(true).unwrap();
        assert_eq!(4, b.len());
        assert_eq!(512, b.capacity());
        let buffer = b.finish();
        assert_eq!(1, buffer.len());

        let mut b = BufferBuilder::<bool>::new(4);
        b.push_slice(&[false, true, false, true]).unwrap();
        assert_eq!(4, b.len());
        assert_eq!(512, b.capacity());
        let buffer = b.finish();
        assert_eq!(1, buffer.len());
    }

    #[test]
    fn test_write_bytes_i32() {
        let mut b = BufferBuilder::<i32>::new(4);
        let bytes = [8, 16, 32, 64].to_byte_slice();
        b.write_bytes(bytes, 4).unwrap();
        assert_eq!(4, b.len());
        assert_eq!(16, b.capacity());
        let buffer = b.finish();
        assert_eq!(16, buffer.len());
    }

    #[test]
    #[should_panic(expected = "Could not write to Buffer, not big enough")]
    fn test_write_too_many_bytes() {
        let mut b = BufferBuilder::<i32>::new(0);
        let bytes = [8, 16, 32, 64].to_byte_slice();
        b.write_bytes(bytes, 4).unwrap();
    }

    #[test]
    fn test_boolean_builder_increases_buffer_len() {
        // 00000010 01001000
        let buf = Buffer::from([72_u8, 2_u8]);
        let mut builder = BufferBuilder::<bool>::new(8);

        for i in 0..10 {
            if i == 3 || i == 6 || i == 9 {
                builder.push(true).unwrap();
            } else {
                builder.push(false).unwrap();
            }
        }
        let buf2 = builder.finish();

        assert_eq!(buf.len(), buf2.len());
        assert_eq!(buf.data(), buf2.data());
    }

    #[test]
    fn test_primitive_array_builder_i32() {
        let mut builder = PrimitiveArray::<i32>::builder(5);
        for i in 0..5 {
            builder.push(i).unwrap();
        }
        let arr = builder.finish();
        assert_eq!(5, arr.len());
        assert_eq!(0, arr.offset());
        assert_eq!(0, arr.null_count());
        for i in 0..5 {
            assert!(!arr.is_null(i));
            assert!(arr.is_valid(i));
            assert_eq!(i as i32, arr.value(i));
        }
    }

    #[test]
    fn test_primitive_array_builder_bool() {
        // 00000010 01001000
        let buf = Buffer::from([72_u8, 2_u8]);
        let mut builder = PrimitiveArray::<bool>::builder(10);
        for i in 0..10 {
            if i == 3 || i == 6 || i == 9 {
                builder.push(true).unwrap();
            } else {
                builder.push(false).unwrap();
            }
        }

        let arr = builder.finish();
        assert_eq!(buf, arr.values());
        assert_eq!(10, arr.len());
        assert_eq!(0, arr.offset());
        assert_eq!(0, arr.null_count());
        for i in 0..10 {
            assert!(!arr.is_null(i));
            assert!(arr.is_valid(i));
            assert_eq!(i == 3 || i == 6 || i == 9, arr.value(i), "failed at {}", i)
        }
    }

    #[test]
    fn test_primitive_array_builder_push_option() {
        let arr1 = PrimitiveArray::<i32>::from(vec![Some(0), None, Some(2), None, Some(4)]);

        let mut builder = PrimitiveArray::<i32>::builder(5);
        builder.push_option(Some(0)).unwrap();
        builder.push_option(None).unwrap();
        builder.push_option(Some(2)).unwrap();
        builder.push_option(None).unwrap();
        builder.push_option(Some(4)).unwrap();
        let arr2 = builder.finish();

        assert_eq!(arr1.len(), arr2.len());
        assert_eq!(arr1.offset(), arr2.offset());
        assert_eq!(arr1.null_count(), arr2.null_count());
        for i in 0..5 {
            assert_eq!(arr1.is_null(i), arr2.is_null(i));
            assert_eq!(arr1.is_valid(i), arr2.is_valid(i));
            if arr1.is_valid(i) {
                assert_eq!(arr1.value(i), arr2.value(i));
            }
        }
    }

    #[test]
    fn test_primitive_array_builder_push_null() {
        let arr1 = PrimitiveArray::<i32>::from(vec![Some(0), Some(2), None, None, Some(4)]);

        let mut builder = PrimitiveArray::<i32>::builder(5);
        builder.push(0).unwrap();
        builder.push(2).unwrap();
        builder.push_null().unwrap();
        builder.push_null().unwrap();
        builder.push(4).unwrap();
        let arr2 = builder.finish();

        assert_eq!(arr1.len(), arr2.len());
        assert_eq!(arr1.offset(), arr2.offset());
        assert_eq!(arr1.null_count(), arr2.null_count());
        for i in 0..5 {
            assert_eq!(arr1.is_null(i), arr2.is_null(i));
            assert_eq!(arr1.is_valid(i), arr2.is_valid(i));
            if arr1.is_valid(i) {
                assert_eq!(arr1.value(i), arr2.value(i));
            }
        }
    }

    #[test]
    fn test_primitive_array_builder_push_slice() {
        let arr1 = PrimitiveArray::<i32>::from(vec![Some(0), Some(2), None, None, Some(4)]);

        let mut builder = PrimitiveArray::<i32>::builder(5);
        builder.push_slice(&[0, 2]).unwrap();
        builder.push_null().unwrap();
        builder.push_null().unwrap();
        builder.push(4).unwrap();
        let arr2 = builder.finish();

        assert_eq!(arr1.len(), arr2.len());
        assert_eq!(arr1.offset(), arr2.offset());
        assert_eq!(arr1.null_count(), arr2.null_count());
        for i in 0..5 {
            assert_eq!(arr1.is_null(i), arr2.is_null(i));
            assert_eq!(arr1.is_valid(i), arr2.is_valid(i));
            if arr1.is_valid(i) {
                assert_eq!(arr1.value(i), arr2.value(i));
            }
        }
    }

    #[test]
    fn test_list_array_builder() {
        let values_builder = PrimitiveArrayBuilder::<i32>::new(10);
        let mut builder = ListArrayBuilder::new(values_builder);

        //  [[0, 1, 2], [3, 4, 5], [6, 7]]
        builder.values().push(0).unwrap();
        builder.values().push(1).unwrap();
        builder.values().push(2).unwrap();
        builder.append(true).unwrap();
        builder.values().push(3).unwrap();
        builder.values().push(4).unwrap();
        builder.values().push(5).unwrap();
        builder.append(true).unwrap();
        builder.values().push(6).unwrap();
        builder.values().push(7).unwrap();
        builder.append(true).unwrap();
        let list_array = builder.finish();

        let values = list_array.values().data().buffers()[0].clone();
        assert_eq!(
            Buffer::from(&[0, 1, 2, 3, 4, 5, 6, 7].to_byte_slice()),
            values
        );
        assert_eq!(
            Buffer::from(&[0, 3, 6, 8].to_byte_slice()),
            list_array.data().buffers()[0].clone()
        );
        assert_eq!(DataType::Int32, list_array.value_type());
        assert_eq!(3, list_array.len());
        assert_eq!(0, list_array.null_count());
        assert_eq!(6, list_array.value_offset(2));
        assert_eq!(2, list_array.value_length(2));
        for i in 0..3 {
            assert!(list_array.is_valid(i as i64));
            assert!(!list_array.is_null(i as i64));
        }
    }

    #[test]
    fn test_list_array_builder_nulls() {
        let values_builder = PrimitiveArrayBuilder::<i32>::new(10);
        let mut builder = ListArrayBuilder::new(values_builder);

        //  [[0, 1, 2], null, [3, null, 5], [6, 7]]
        builder.values().push(0).unwrap();
        builder.values().push(1).unwrap();
        builder.values().push(2).unwrap();
        builder.append(true).unwrap();
        builder.append(false).unwrap();
        builder.values().push(3).unwrap();
        builder.values().push_null().unwrap();
        builder.values().push(5).unwrap();
        builder.append(true).unwrap();
        builder.values().push(6).unwrap();
        builder.values().push(7).unwrap();
        builder.append(true).unwrap();
        let list_array = builder.finish();

        assert_eq!(DataType::Int32, list_array.value_type());
        assert_eq!(4, list_array.len());
        assert_eq!(1, list_array.null_count());
        assert_eq!(3, list_array.value_offset(2));
        assert_eq!(3, list_array.value_length(2));
    }

    #[test]
    fn test_list_list_array_builder() {
        let primitive_builder = PrimitiveArrayBuilder::<i32>::new(10);
        let values_builder = ListArrayBuilder::new(primitive_builder);
        let mut builder = ListArrayBuilder::new(values_builder);

        //  [[[1, 2], [3, 4]], [[5, 6, 7], null, [8]], null, [[9, 10]]]
        builder.values().values().push(1).unwrap();
        builder.values().values().push(2).unwrap();
        builder.values().append(true).unwrap();
        builder.values().values().push(3).unwrap();
        builder.values().values().push(4).unwrap();
        builder.values().append(true).unwrap();
        builder.append(true).unwrap();

        builder.values().values().push(5).unwrap();
        builder.values().values().push(6).unwrap();
        builder.values().values().push(7).unwrap();
        builder.values().append(true).unwrap();
        builder.values().append(false).unwrap();
        builder.values().values().push(8).unwrap();
        builder.values().append(true).unwrap();
        builder.append(true).unwrap();

        builder.append(false).unwrap();

        builder.values().values().push(9).unwrap();
        builder.values().values().push(10).unwrap();
        builder.values().append(true).unwrap();
        builder.append(true).unwrap();

        let list_array = builder.finish();

        assert_eq!(4, list_array.len());
        assert_eq!(1, list_array.null_count());
        assert_eq!(
            Buffer::from(&[0, 2, 5, 5, 6].to_byte_slice()),
            list_array.data().buffers()[0].clone()
        );

        assert_eq!(6, list_array.values().data().len());
        assert_eq!(1, list_array.values().data().null_count());
        assert_eq!(
            Buffer::from(&[0, 2, 4, 7, 7, 8, 10].to_byte_slice()),
            list_array.values().data().buffers()[0].clone()
        );

        assert_eq!(10, list_array.values().data().child_data()[0].len());
        assert_eq!(0, list_array.values().data().child_data()[0].null_count());
        assert_eq!(
            Buffer::from(&[1, 2, 3, 4, 5, 6, 7, 8, 9, 10].to_byte_slice()),
            list_array.values().data().child_data()[0].buffers()[0].clone()
        );
    }
}
