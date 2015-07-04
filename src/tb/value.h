/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_VALUE_H_
#define TB_VALUE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "tb/config.h"

namespace tb {

class Value;
namespace util {
class TypedObject;
}  // namespace util

// ValueArray is an array of Value.
class ValueArray {
 public:
  ValueArray();
  ~ValueArray();

  static ValueArray* Clone(ValueArray* source);

  Value* AddValue();
  Value* AddInteger(int32_t value);

  size_t size() const { return list_.size(); }
  Value* at(size_t i) const;
  inline Value* operator[](size_t i) const { return at(i); }

 private:
  std::vector<std::unique_ptr<Value>> list_;
};

// Value holds value of a specific type.
// In addition to NULL, string, float, integer, it may also contain an array of
// attributes (ValueArray), or an object (derived from TypedObject).
// When getting the value as a different type from what it is, it may convert
// its internal representation to that type. Exceptions are for array and
// object, which will return 0 when getting as numbers, or "" or object name
// when getting as string.
class Value {
 public:
  // The current type of the value.
  // It may change when using a getter of a different type.
  enum class Type {
    kNull,
    kString,
    kFloat,
    kInt,
    kObject,
    kArray,
  };

  // How to deal with the dynamic memory when setting string and array.
  enum class Set {
    kNewCopy,        // A new copy of the data will be made.
    kTakeOwnership,  // The data passed in will be stored and freed.
    kAsStatic        // The data passed in will be stored but never freed.
  };

  Value();
  Value(const Value& value);
  Value(Type type);

  Value(int value);
  Value(float value);
  Value(const char* value, Set set = Set::kNewCopy);
  Value(util::TypedObject* object);

  ~Value();

  // Takes over ownership of content of source_value.
  // NOTE: If source_value has string or array that are set with Set::kAsStatic,
  // it will make new copies of those.
  // NOTE: value will be nulled on source_value after this call.
  void TakeOver(Value& source_value);

  // Copies the content of source_value to this value.
  // NOTE: This value will become Type::kNull if source_value holds an object.
  // We can't copy objects.
  void Copy(const Value& source_value);

  void reset() { set_null(); }
  void set_null();
  void set_integer(int val);
  void set_float(float val);

  // Sets the passed in string.
  void set_string(const char* val, Set set);
  void set_string(const std::string& val) {
    set_string(val.c_str(), Set::kNewCopy);
  }

  // Sets the passed in object. Takes the ownership of the object!
  void set_object(util::TypedObject* object);

  // Sets the passed in array.
  void set_array(ValueArray* arr, Set set);

  // Sets the value either as a string, number or array of numbers, depending of
  // the string syntax.
  void parse_string(const char* str, Set set);

  int as_integer() const;
  float as_float() const;
  const char* as_string();
  util::TypedObject* as_object() const {
    return is_object() ? value_.object : nullptr;
  }
  ValueArray* as_array() const {
    return is_array() ? value_.value_array : nullptr;
  }

  std::string to_string() const;

  Type type() const { return Type(packed_.type); }
  bool is_string() const { return Type(packed_.type) == Type::kString; }
  bool is_float() const { return Type(packed_.type) == Type::kFloat; }
  bool is_integer() const { return Type(packed_.type) == Type::kInt; }
  bool is_object() const { return Type(packed_.type) == Type::kObject; }
  bool is_array() const { return Type(packed_.type) == Type::kArray; }
  size_t array_size() const {
    return is_array() ? value_.value_array->size() : 0;
  }

  const Value& operator=(const Value& val) {
    Copy(val);
    return *this;
  }

 private:
  void set_type(Type type) { packed_.type = uint32_t(type); }

  union {
    float float_number;
    int integer_number;
    char* string;
    util::TypedObject* object;
    ValueArray* value_array = nullptr;
  } value_;
  union {
    struct {
      uint32_t type : 8;
      uint32_t allocated : 1;
    } packed_;
    uint32_t packed_init_ = 0;
  };
};

}  // namespace tb

#endif  // TB_VALUE_H_
