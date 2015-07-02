/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_VALUE_H
#define TB_VALUE_H

#include <cstdint>

#include "tb_core.h"
#include "tb_list.h"
#include "tb_str.h"

namespace tb {

class TBValue;
class TBTypedObject;

// Return true if the given string starts with a number.
// Ex: 100, -.2, 1.0E-8, 5px will all return true.
bool is_start_of_number(const char* str);

// Returns true if the given string contains space that is not at the end of the
// string.
bool contains_non_trailing_space(const char* str);

// Return true if the string can be represented as a number.
// It ignores trailing white space.
// Ex: 100, -.2 will return true.
// Ex: 1.0E-8, 5px will return false.
bool is_number_only(const char* str);

// Return true if the given number string is a float number.
// Should only be called when you've verified it's a number with is_number().
bool is_number_float(const char* str);

// TBValueArray is a array of TBValue.
class TBValueArray {
 public:
  TBValueArray();
  ~TBValueArray();
  TBValue* AddValue();
  TBValue* GetValue(int index);
  static TBValueArray* Clone(TBValueArray* source);
  int GetLength() const { return m_list.GetNumItems(); }

 private:
  TBListAutoDeleteOf<TBValue> m_list;
};

// TBValue holds value of a specific type.
// In addition to NULL, string, float, integer, it may also contain an array of
// attributes (TBValueArray), or an object (derived from TBTypedObject).
// When getting the value as a different type from what it is, it may convert
// its internal representation to that type. Exceptions are for array and
// object, which will return 0 when getting as numbers, or "" or object name
// when getting as string.
class TBValue {
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

  TBValue();
  TBValue(const TBValue& value);
  TBValue(Type type);

  TBValue(int value);
  TBValue(float value);
  TBValue(const char* value, Set set = Set::kNewCopy);
  TBValue(TBTypedObject* object);

  ~TBValue();

  // Takes over ownership of content of source_value.
  // NOTE: If source_value has string or array that are set with Set::kAsStatic,
  // it will make new copies of those.
  // NOTE: value will be nulled on source_value after this call.
  void TakeOver(TBValue& source_value);

  // Copies the content of source_value to this value.
  // NOTE: This value will become Type::kNull if source_value holds an object.
  // We can't copy objects.
  void Copy(const TBValue& source_value);

  void SetNull();
  void SetInt(int val);
  void SetFloat(float val);

  // Sets the passed in string.
  void SetString(const char* val, Set set);
  void SetString(const std::string& val) {
    SetString(val.c_str(), Set::kNewCopy);
  }

  // Sets the passed in object. Takes the ownership of the object!
  void SetObject(TBTypedObject* object);

  // Sets the passed in array.
  void SetArray(TBValueArray* arr, Set set);

  // Sets the value either as a string, number or array of numbers, depending of
  // the string syntax.
  void SetFromStringAuto(const char* str, Set set);

  int GetInt() const;
  float GetFloat() const;
  const char* GetString();
  TBTypedObject* GetObject() const { return IsObject() ? val_obj : nullptr; }
  TBValueArray* GetArray() const { return IsArray() ? val_arr : nullptr; }

  Type GetType() const { return Type(m_packed.type); }
  bool IsString() const { return Type(m_packed.type) == Type::kString; }
  bool IsFloat() const { return Type(m_packed.type) == Type::kFloat; }
  bool IsInt() const { return Type(m_packed.type) == Type::kInt; }
  bool IsObject() const { return Type(m_packed.type) == Type::kObject; }
  bool IsArray() const { return Type(m_packed.type) == Type::kArray; }
  int GetArrayLength() const { return IsArray() ? val_arr->GetLength() : 0; }

  const TBValue& operator=(const TBValue& val) {
    Copy(val);
    return *this;
  }

 private:
  void SetType(Type type) { m_packed.type = uint32_t(type); }

  union {
    float val_float;
    int val_int;
    char* val_str;
    TBTypedObject* val_obj;
    TBValueArray* val_arr;
  };
  union {
    struct {
      uint32_t type : 8;
      uint32_t allocated : 1;
    } m_packed;
    uint32_t m_packed_init;
  };
};

}  // namespace tb

#endif  // TB_VALUE_H
