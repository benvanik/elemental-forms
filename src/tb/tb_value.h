/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_VALUE_H
#define TB_VALUE_H

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

  Value* AddValue();
  Value* GetValue(size_t index);
  static ValueArray* Clone(ValueArray* source);
  size_t GetLength() const { return list_.size(); }

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

  void SetNull();
  void SetInt(int val);
  void SetFloat(float val);

  // Sets the passed in string.
  void SetString(const char* val, Set set);
  void SetString(const std::string& val) {
    SetString(val.c_str(), Set::kNewCopy);
  }

  // Sets the passed in object. Takes the ownership of the object!
  void SetObject(util::TypedObject* object);

  // Sets the passed in array.
  void SetArray(ValueArray* arr, Set set);

  // Sets the value either as a string, number or array of numbers, depending of
  // the string syntax.
  void SetFromStringAuto(const char* str, Set set);

  int GetInt() const;
  float GetFloat() const;
  const char* GetString();
  util::TypedObject* GetObject() const {
    return IsObject() ? val_obj : nullptr;
  }
  ValueArray* GetArray() const { return IsArray() ? val_arr : nullptr; }

  Type GetType() const { return Type(m_packed.type); }
  bool IsString() const { return Type(m_packed.type) == Type::kString; }
  bool IsFloat() const { return Type(m_packed.type) == Type::kFloat; }
  bool IsInt() const { return Type(m_packed.type) == Type::kInt; }
  bool IsObject() const { return Type(m_packed.type) == Type::kObject; }
  bool IsArray() const { return Type(m_packed.type) == Type::kArray; }
  size_t GetArrayLength() const { return IsArray() ? val_arr->GetLength() : 0; }

  const Value& operator=(const Value& val) {
    Copy(val);
    return *this;
  }

 private:
  void SetType(Type type) { m_packed.type = uint32_t(type); }

  union {
    float val_float;
    int val_int;
    char* val_str;
    util::TypedObject* val_obj;
    ValueArray* val_arr = nullptr;
  };
  union {
    struct {
      uint32_t type : 8;
      uint32_t allocated : 1;
    } m_packed;
    uint32_t m_packed_init = 0;
  };
};

}  // namespace tb

#endif  // TB_VALUE_H
