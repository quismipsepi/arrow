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

use std::cmp;
use std::io::{Error as IoError, ErrorKind, Result as IoResult, Write};
use std::mem;
use std::sync::Arc;

use error::Result;
use memory;
use util::bit_util;

/// Buffer is a contiguous memory region of fixed size and is aligned at a 64-byte
/// boundary. Buffer is immutable.
#[derive(PartialEq, Debug)]
pub struct Buffer {
    /// Reference-counted pointer to the internal byte buffer.
    data: Arc<BufferData>,

    /// The offset into the buffer.
    offset: usize,
}

#[derive(Debug)]
struct BufferData {
    /// The raw pointer into the buffer bytes
    ptr: *const u8,

    /// The length of the buffer
    len: usize,
}

impl PartialEq for BufferData {
    fn eq(&self, other: &BufferData) -> bool {
        if self.len != other.len {
            return false;
        }
        unsafe { memory::memcmp(self.ptr, other.ptr, self.len as usize) == 0 }
    }
}

/// Release the underlying memory when the current buffer goes out of scope
impl Drop for BufferData {
    fn drop(&mut self) {
        memory::free_aligned(self.ptr);
    }
}

impl Buffer {
    /// Creates a buffer from an existing memory region (must already be byte-aligned)
    pub fn from_raw_parts(ptr: *const u8, len: usize) -> Self {
        assert!(memory::is_aligned(ptr, 64), "memory not aligned");
        let buf_data = BufferData { ptr, len };
        Buffer {
            data: Arc::new(buf_data),
            offset: 0,
        }
    }

    /// Returns the number of bytes in the buffer
    pub fn len(&self) -> usize {
        self.data.len - self.offset as usize
    }

    /// Returns whether the buffer is empty.
    pub fn is_empty(&self) -> bool {
        self.data.len - self.offset == 0
    }

    /// Returns the byte slice stored in this buffer
    pub fn data(&self) -> &[u8] {
        unsafe { ::std::slice::from_raw_parts(self.raw_data(), self.len()) }
    }

    /// Returns a slice of this buffer, starting from `offset`.
    pub fn slice(&self, offset: usize) -> Self {
        assert!(
            self.offset + offset <= self.len(),
            "the offset of the new Buffer cannot exceed the existing length"
        );
        Self {
            data: self.data.clone(),
            offset: self.offset + offset,
        }
    }

    /// Returns a raw pointer for this buffer.
    ///
    /// Note that this should be used cautiously, and the returned pointer should not be
    /// stored anywhere, to avoid dangling pointers.
    pub fn raw_data(&self) -> *const u8 {
        unsafe { self.data.ptr.offset(self.offset as isize) }
    }

    /// Returns an empty buffer.
    pub fn empty() -> Self {
        Self::from_raw_parts(::std::ptr::null(), 0)
    }
}

impl Clone for Buffer {
    fn clone(&self) -> Buffer {
        Buffer {
            data: self.data.clone(),
            offset: self.offset,
        }
    }
}

/// Creating a `Buffer` instance by copying the memory from a `AsRef<[u8]>` into a newly
/// allocated memory region.
impl<T: AsRef<[u8]>> From<T> for Buffer {
    fn from(p: T) -> Self {
        // allocate aligned memory buffer
        let slice = p.as_ref();
        let len = slice.len() * mem::size_of::<u8>();
        let buffer = memory::allocate_aligned((len) as i64).unwrap();
        unsafe {
            memory::memcpy(buffer, slice.as_ptr(), len);
        }
        Buffer::from_raw_parts(buffer, len)
    }
}

unsafe impl Sync for Buffer {}
unsafe impl Send for Buffer {}

/// Similar to `Buffer`, but is growable and can be mutated. A mutable buffer can be
/// converted into a immutable buffer via the `freeze` method.
#[derive(Debug)]
pub struct MutableBuffer {
    data: *mut u8,
    len: usize,
    capacity: usize,
}

impl MutableBuffer {
    /// Allocate a new mutable buffer with initial capacity to be `capacity`.
    pub fn new(capacity: usize) -> Self {
        let new_capacity = bit_util::round_upto_multiple_of_64(capacity as i64);
        let ptr = memory::allocate_aligned(new_capacity).unwrap();
        Self {
            data: ptr,
            len: 0,
            capacity: new_capacity as usize,
        }
    }

    /// Adjust the capacity of this buffer to be at least `new_capacity`.
    ///
    /// If the `new_capacity` is less than the current capacity, nothing is done and `Ok`
    /// will be returned. Otherwise, the new capacity value will be chosen between the
    /// larger one of the incoming `new_capacity` (after rounding up to the nearest 64)
    /// and the doubled value of the existing capacity.
    pub fn resize(&mut self, new_capacity: usize) -> Result<()> {
        if new_capacity <= self.capacity {
            return Ok(());
        }
        let new_capacity = bit_util::round_upto_multiple_of_64(new_capacity as i64);
        let new_capacity = cmp::max(new_capacity, self.capacity as i64 * 2);
        let new_data = memory::reallocate(self.capacity, new_capacity as usize, self.data)?;
        self.data = new_data as *mut u8;
        self.capacity = new_capacity as usize;
        Ok(())
    }

    /// Returns whether this buffer is empty or not.
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    /// Returns the length (the number of bytes written) in this buffer.
    pub fn len(&self) -> usize {
        self.len
    }

    /// Returns the total capacity in this buffer.
    pub fn capacity(&self) -> usize {
        self.capacity
    }

    /// Clear all existing data from this buffer.
    pub fn clear(&mut self) {
        self.len = 0
    }

    /// Returns the data stored in this buffer as a slice.
    pub fn data(&self) -> &[u8] {
        unsafe { ::std::slice::from_raw_parts(self.raw_data(), self.len()) }
    }

    /// Returns a raw pointer for this buffer.
    ///
    /// Note that this should be used cautiously, and the returned pointer should not be
    /// stored anywhere, to avoid dangling pointers.
    pub fn raw_data(&self) -> *const u8 {
        self.data
    }

    /// Freezes this buffer and return an immutable version of it.
    pub fn freeze(self) -> Buffer {
        let buffer_data = BufferData {
            ptr: self.data,
            len: self.len,
        };
        ::std::mem::forget(self);
        Buffer {
            data: Arc::new(buffer_data),
            offset: 0,
        }
    }
}

impl Drop for MutableBuffer {
    fn drop(&mut self) {
        memory::free_aligned(self.data);
    }
}

impl PartialEq for MutableBuffer {
    fn eq(&self, other: &MutableBuffer) -> bool {
        if self.len != other.len {
            return false;
        }
        unsafe { memory::memcmp(self.data, other.data, self.len as usize) == 0 }
    }
}

impl Write for MutableBuffer {
    fn write(&mut self, buf: &[u8]) -> IoResult<usize> {
        let remaining_capacity = self.capacity - self.len;
        if buf.len() > remaining_capacity {
            return Err(IoError::new(ErrorKind::Other, "Buffer not big enough"));
        }
        unsafe {
            memory::memcpy(self.data.offset(self.len as isize), buf.as_ptr(), buf.len());
            self.len += buf.len();
            Ok(buf.len())
        }
    }

    fn flush(&mut self) -> IoResult<()> {
        Ok(())
    }
}

unsafe impl Sync for MutableBuffer {}
unsafe impl Send for MutableBuffer {}

#[cfg(test)]
mod tests {
    use std::ptr::null_mut;
    use std::thread;

    use super::*;

    #[test]
    fn test_buffer_data_equality() {
        let buf1 = Buffer::from(&[0, 1, 2, 3, 4]);
        let mut buf2 = Buffer::from(&[0, 1, 2, 3, 4]);
        assert_eq!(buf1, buf2);

        // slice with same offset should still preserve equality
        let buf3 = buf1.slice(2);
        assert_ne!(buf1, buf3);
        let buf4 = buf2.slice(2);
        assert_eq!(buf3, buf4);

        // unequal because of different elements
        buf2 = Buffer::from(&[0, 0, 2, 3, 4]);
        assert_ne!(buf1, buf2);

        // unequal because of different length
        buf2 = Buffer::from(&[0, 1, 2, 3]);
        assert_ne!(buf1, buf2);
    }

    #[test]
    fn test_from_raw_parts() {
        let buf = Buffer::from_raw_parts(null_mut(), 0);
        assert_eq!(0, buf.len());
        assert_eq!(0, buf.data().len());
        assert!(buf.raw_data().is_null());

        let buf = Buffer::from(&[0, 1, 2, 3, 4]);
        assert_eq!(5, buf.len());
        assert!(!buf.raw_data().is_null());
        assert_eq!(&[0, 1, 2, 3, 4], buf.data());
    }

    #[test]
    fn test_from_vec() {
        let buf = Buffer::from(&[0, 1, 2, 3, 4]);
        assert_eq!(5, buf.len());
        assert!(!buf.raw_data().is_null());
        assert_eq!(&[0, 1, 2, 3, 4], buf.data());
    }

    #[test]
    fn test_copy() {
        let buf = Buffer::from(&[0, 1, 2, 3, 4]);
        let buf2 = buf.clone();
        assert_eq!(5, buf2.len());
        assert!(!buf2.raw_data().is_null());
        assert_eq!(&[0, 1, 2, 3, 4], buf2.data());
    }

    #[test]
    fn test_slice() {
        let buf = Buffer::from(&[2, 4, 6, 8, 10]);
        let buf2 = buf.slice(2);

        assert_eq!(&[6, 8, 10], buf2.data());
        assert_eq!(3, buf2.len());
        assert_eq!(unsafe { buf.raw_data().offset(2) }, buf2.raw_data());

        let buf3 = buf2.slice(1);
        assert_eq!(&[8, 10], buf3.data());
        assert_eq!(2, buf3.len());
        assert_eq!(unsafe { buf.raw_data().offset(3) }, buf3.raw_data());

        let buf4 = buf.slice(5);
        let empty_slice: [u8; 0] = [];
        assert_eq!(empty_slice, buf4.data());
        assert_eq!(0, buf4.len());
        assert!(buf4.is_empty());
    }

    #[test]
    #[should_panic(expected = "the offset of the new Buffer cannot exceed the existing length")]
    fn test_slice_offset_out_of_bound() {
        let buf = Buffer::from(&[2, 4, 6, 8, 10]);
        buf.slice(6);
    }

    #[test]
    fn test_mutable_new() {
        let buf = MutableBuffer::new(63);
        assert_eq!(64, buf.capacity());
        assert_eq!(0, buf.len());
        assert!(buf.is_empty());
    }

    #[test]
    fn test_mutable_write() {
        let mut buf = MutableBuffer::new(100);
        buf.write("hello".as_bytes()).expect("Ok");
        assert_eq!(5, buf.len());
        assert_eq!("hello".as_bytes(), buf.data());

        buf.write(" world".as_bytes()).expect("Ok");
        assert_eq!(11, buf.len());
        assert_eq!("hello world".as_bytes(), buf.data());

        buf.clear();
        assert_eq!(0, buf.len());
        buf.write("hello arrow".as_bytes()).expect("Ok");
        assert_eq!(11, buf.len());
        assert_eq!("hello arrow".as_bytes(), buf.data());
    }

    #[test]
    #[should_panic(expected = "Buffer not big enough")]
    fn test_mutable_write_overflow() {
        let mut buf = MutableBuffer::new(1);
        assert_eq!(64, buf.capacity());
        for _ in 0..10 {
            buf.write(&[0, 0, 0, 0, 0, 0, 0, 0]).unwrap();
        }
    }

    #[test]
    fn test_mutable_resize() {
        let mut buf = MutableBuffer::new(1);
        assert_eq!(64, buf.capacity());

        // resizing to a smaller value should have no effect.
        buf.resize(10).expect("resize should be OK");
        assert_eq!(64, buf.capacity());

        buf.resize(100).expect("resize should be OK");
        assert_eq!(128, buf.capacity());
    }

    #[test]
    fn test_mutable_freeze() {
        let mut buf = MutableBuffer::new(1);
        buf.write("aaaa bbbb cccc dddd".as_bytes())
            .expect("write should be OK");
        assert_eq!(19, buf.len());
        assert_eq!("aaaa bbbb cccc dddd".as_bytes(), buf.data());

        let immutable_buf = buf.freeze();
        assert_eq!(19, immutable_buf.len());
        assert_eq!("aaaa bbbb cccc dddd".as_bytes(), immutable_buf.data());
    }

    #[test]
    fn test_access_concurrently() {
        let buffer = Buffer::from(vec![1, 2, 3, 4, 5]);
        let buffer2 = buffer.clone();
        assert_eq!(&[1, 2, 3, 4, 5], buffer.data());

        let buffer_copy = thread::spawn(move || {
            // access buffer in another thread.
            buffer.clone()
        }).join();

        assert!(buffer_copy.is_ok());
        assert_eq!(buffer2, buffer_copy.ok().unwrap());
    }
}
