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

#ifndef ARROW_TYPE_H
#define ARROW_TYPE_H

#include <memory>
#include <string>

namespace arrow {

// Physical data type that describes the memory layout of values. See details
// for each type
enum class LayoutEnum: char {
  // A physical type consisting of some non-negative number of bytes
  BYTE = 0,

  // A physical type consisting of some non-negative number of bits
  BIT = 1,

  // A parametric variable-length value type. Full specification requires a
  // child logical type
  LIST = 2,

  // A collection of multiple equal-length child arrays. Parametric type taking
  // 1 or more child logical types
  STRUCT = 3,

  // An array with heterogeneous value types. Parametric types taking 1 or more
  // child logical types
  DENSE_UNION = 4,
  SPARSE_UNION = 5
};


struct LayoutType {
  LayoutEnum type;
  explicit LayoutType(LayoutEnum type) : type(type) {}
};

// Data types in this library are all *logical*. They can be expressed as
// either a primitive physical type (bytes or bits of some fixed size), a
// nested type consisting of other data types, or another data type (e.g. a
// timestamp encoded as an int64)
struct LogicalType {
  enum type {
    // A degenerate NULL type represented as 0 bytes/bits
    NA = 0,

    // Little-endian integer types
    UINT8 = 1,
    INT8 = 2,
    UINT16 = 3,
    INT16 = 4,
    UINT32 = 5,
    INT32 = 6,
    UINT64 = 7,
    INT64 = 8,

    // A boolean value represented as 1 byte
    BOOL = 9,

    // A boolean value represented as 1 bit
    BIT = 10,

    // 4-byte floating point value
    FLOAT = 11,

    // 8-byte floating point value
    DOUBLE = 12,

    // CHAR(N): fixed-length UTF8 string with length N
    CHAR = 13,

    // UTF8 variable-length string as List<Char>
    STRING = 14,

    // VARCHAR(N): Null-terminated string type embedded in a CHAR(N + 1)
    VARCHAR = 15,

    // Variable-length bytes (no guarantee of UTF8-ness)
    BINARY = 16,

    // By default, int32 days since the UNIX epoch
    DATE = 17,

    // Exact timestamp encoded with int64 since UNIX epoch
    // Default unit millisecond
    TIMESTAMP = 18,

    // Timestamp as double seconds since the UNIX epoch
    TIMESTAMP_DOUBLE = 19,

    // Exact time encoded with int64, default unit millisecond
    TIME = 20,

    // Precision- and scale-based decimal type. Storage type depends on the
    // parameters.
    DECIMAL = 21,

    // Decimal value encoded as a text string
    DECIMAL_TEXT = 22,

    // A list of some logical data type
    LIST = 30,

    // Struct of logical types
    STRUCT = 31,

    // Unions of logical types
    DENSE_UNION = 32,
    SPARSE_UNION = 33,

    // Union<Null, Int32, Double, String, Bool>
    JSON_SCALAR = 50,

    // User-defined type
    USER = 60
  };
};

struct DataType {
  LogicalType::type type;
  bool nullable;

  explicit DataType(LogicalType::type type, bool nullable = true) :
      type(type),
      nullable(nullable) {}

  virtual bool Equals(const DataType* other) {
    return this == other || (this->type == other->type &&
        this->nullable == other->nullable);
  }

  virtual std::string ToString() const = 0;
};


typedef std::shared_ptr<LayoutType> LayoutPtr;
typedef std::shared_ptr<DataType> TypePtr;


struct BytesType : public LayoutType {
  int size;

  explicit BytesType(int size)
      : LayoutType(LayoutEnum::BYTE),
        size(size) {}

  BytesType(const BytesType& other)
      : BytesType(other.size) {}
};

struct ListLayoutType : public LayoutType {
  LayoutPtr value_type;

  explicit ListLayoutType(const LayoutPtr& value_type)
      : LayoutType(LayoutEnum::BYTE),
        value_type(value_type) {}
};

template <typename Derived>
struct PrimitiveType : public DataType {
  explicit PrimitiveType(bool nullable = true)
      : DataType(Derived::type_enum, nullable) {}

  virtual std::string ToString() const {
    std::string result;
    if (nullable) {
      result.append("?");
    }
    result.append(static_cast<const Derived*>(this)->name());
    return result;
  }
};

#define PRIMITIVE_DECL(TYPENAME, C_TYPE, ENUM, SIZE, NAME)          \
  typedef C_TYPE c_type;                                            \
  static constexpr LogicalType::type type_enum = LogicalType::ENUM; \
  static constexpr int size = SIZE;                                 \
                                                                    \
  explicit TYPENAME(bool nullable = true)                           \
      : PrimitiveType<TYPENAME>(nullable) {}                        \
                                                                    \
  static const char* name() {                                       \
    return NAME;                                                    \
  }

struct BooleanType : public PrimitiveType<BooleanType> {
  PRIMITIVE_DECL(BooleanType, uint8_t, BOOL, 1, "bool");
};

struct UInt8Type : public PrimitiveType<UInt8Type> {
  PRIMITIVE_DECL(UInt8Type, uint8_t, UINT8, 1, "uint8");
};

struct Int8Type : public PrimitiveType<Int8Type> {
  PRIMITIVE_DECL(Int8Type, int8_t, INT8, 1, "int8");
};

struct UInt16Type : public PrimitiveType<UInt16Type> {
  PRIMITIVE_DECL(UInt16Type, uint16_t, UINT16, 2, "uint16");
};

struct Int16Type : public PrimitiveType<Int16Type> {
  PRIMITIVE_DECL(Int16Type, int16_t, INT16, 2, "int16");
};

struct UInt32Type : public PrimitiveType<UInt32Type> {
  PRIMITIVE_DECL(UInt32Type, uint32_t, UINT32, 4, "uint32");
};

struct Int32Type : public PrimitiveType<Int32Type> {
  PRIMITIVE_DECL(Int32Type, int32_t, INT32, 4, "int32");
};

struct UInt64Type : public PrimitiveType<UInt64Type> {
  PRIMITIVE_DECL(UInt64Type, uint64_t, UINT64, 8, "uint64");
};

struct Int64Type : public PrimitiveType<Int64Type> {
  PRIMITIVE_DECL(Int64Type, int64_t, INT64, 8, "int64");
};

struct FloatType : public PrimitiveType<FloatType> {
  PRIMITIVE_DECL(FloatType, float, FLOAT, 4, "float");
};

struct DoubleType : public PrimitiveType<DoubleType> {
  PRIMITIVE_DECL(DoubleType, double, DOUBLE, 8, "double");
};

} // namespace arrow

#endif  // ARROW_TYPE_H
