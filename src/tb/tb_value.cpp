/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_value.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tb/util/object.h"

namespace tb {

// FIX: ## Floating point string conversions might be locale dependant. Force
// "." as decimal!

char* next_token(char*& str, const char* delim) {
  str += strspn(str, delim);
  if (!*str) return nullptr;
  char* token = str;
  str += strcspn(str, delim);
  if (*str) *str++ = '\0';
  return token;
}

bool is_start_of_number(const char* str) {
  if (*str == '-') str++;
  if (*str == '.') str++;
  return *str >= '0' && *str <= '9';
}

bool contains_non_trailing_space(const char* str) {
  if (const char* p = strstr(str, " ")) {
    while (*p == ' ') p++;
    return *p != '\0';
  }
  return false;
}

bool is_number_only(const char* s) {
  if (!s || *s == 0 || *s == ' ') return 0;
  char* p;
  strtod(s, &p);
  while (*p == ' ') p++;
  return *p == '\0';
}

bool is_number_float(const char* str) {
  while (*str)
    if (*str++ == '.') return true;
  return false;
}

ValueArray::ValueArray() = default;

ValueArray::~ValueArray() = default;

Value* ValueArray::AddValue() {
  list_.push_back(std::make_unique<Value>());
  return list_.back().get();
}

Value* ValueArray::GetValue(size_t index) {
  if (index >= 0 && index < list_.size()) {
    return list_[index].get();
  }
  return nullptr;
}

ValueArray* ValueArray::Clone(ValueArray* source) {
  ValueArray* new_arr = new ValueArray();
  for (auto& it : source->list_) {
    Value* new_val = new_arr->AddValue();
    new_val->Copy(*it.get());
  }
  return new_arr;
}

Value::Value() = default;

Value::Value(const Value& value) { Copy(value); }

Value::Value(Type type) {
  switch (type) {
    case Type::kNull:
      SetNull();
      break;
    case Type::kString:
      SetString("", Set::kAsStatic);
      break;
    case Type::kFloat:
      SetFloat(0);
      break;
    case Type::kInt:
      SetInt(0);
      break;
    case Type::kObject:
      SetObject(nullptr);
      break;
    case Type::kArray:
      SetArray(new ValueArray(), Set::kTakeOwnership);
      break;
    default:
      assert(!"Not implemented!");
      break;
  };
}

Value::Value(int value) { SetInt(value); }

Value::Value(float value) { SetFloat(value); }

Value::Value(const char* value, Set set) { SetString(value, set); }

Value::Value(util::TypedObject* object) { SetObject(object); }

Value::~Value() { SetNull(); }

void Value::TakeOver(Value& source_value) {
  if (Type(source_value.m_packed.type) == Type::kString) {
    SetString(source_value.val_str, source_value.m_packed.allocated
                                        ? Set::kTakeOwnership
                                        : Set::kNewCopy);
  } else if (source_value.GetType() == Type::kArray) {
    SetArray(source_value.val_arr, source_value.m_packed.allocated
                                       ? Set::kTakeOwnership
                                       : Set::kNewCopy);
  } else {
    *this = source_value;
  }
  source_value.SetType(Type::kNull);
}

void Value::Copy(const Value& source_value) {
  if (source_value.GetType() == Type::kString) {
    SetString(source_value.val_str, Set::kNewCopy);
  } else if (source_value.GetType() == Type::kArray) {
    SetArray(source_value.val_arr, Set::kNewCopy);
  } else if (source_value.GetType() == Type::kObject) {
    assert(!"We can't copy objects! The value will be nulled!");
    SetObject(nullptr);
  } else {
    SetNull();
    std::memcpy(this, &source_value, sizeof(Value));
  }
}

void Value::SetNull() {
  if (m_packed.allocated) {
    if (GetType() == Type::kString) {
      free(val_str);
      val_str = nullptr;
    } else if (GetType() == Type::kObject) {
      delete val_obj;
      val_obj = nullptr;
    } else if (GetType() == Type::kArray) {
      delete val_arr;
      val_arr = nullptr;
    }
  }
  SetType(Type::kNull);
}

void Value::SetInt(int val) {
  SetNull();
  SetType(Type::kInt);
  val_int = val;
}

void Value::SetFloat(float val) {
  SetNull();
  SetType(Type::kFloat);
  val_float = val;
}

void Value::SetString(const char* val, Set set) {
  SetNull();
  m_packed.allocated = set == Set::kNewCopy || set == Set::kTakeOwnership;
  if (set != Set::kNewCopy) {
    val_str = const_cast<char*>(val);
    SetType(Type::kString);
  } else if ((val_str = strdup(val))) {
    SetType(Type::kString);
  }
}

void Value::SetObject(util::TypedObject* object) {
  SetNull();
  SetType(Type::kObject);
  m_packed.allocated = true;
  val_obj = object;
}

void Value::SetArray(ValueArray* arr, Set set) {
  SetNull();
  m_packed.allocated = set == Set::kNewCopy || set == Set::kTakeOwnership;
  if (set != Set::kNewCopy) {
    val_arr = arr;
    SetType(Type::kArray);
  } else if ((val_arr = ValueArray::Clone(arr))) {
    SetType(Type::kArray);
  }
}

void Value::SetFromStringAuto(const char* str, Set set) {
  if (!str) {
    SetNull();
  } else if (is_number_only(str)) {
    if (is_number_float(str)) {
      SetFloat((float)atof(str));
    } else {
      SetInt(atoi(str));
    }
  } else if (is_start_of_number(str) && contains_non_trailing_space(str)) {
    // If the number has nontrailing space, we'll assume a list of numbers.
    // (example: "10 -4 3.5")
    SetNull();
    ValueArray* arr = new ValueArray();
    std::string tmpstr = str;
    char* str_next = (char*)tmpstr.data();
    while (char* token = next_token(str_next, ", ")) {
      Value* new_val = arr->AddValue();
      new_val->SetFromStringAuto(token, Set::kNewCopy);
    }
    SetArray(arr, Set::kTakeOwnership);
  } else if (*str == '[') {
    SetNull();
    ValueArray* arr = new ValueArray();
    assert(!"not implemented! Split out the tokenizer code above!");
    SetArray(arr, Set::kTakeOwnership);
  } else {
    SetString(str, set);
    return;
  }
  // We didn't set as string, so we might need to deal with the passed in string
  // data.
  if (set == Set::kTakeOwnership) {
    // Delete the passed in data.
    Value tmp;
    tmp.SetString(str, Set::kTakeOwnership);
  }
}

int Value::GetInt() const {
  if (GetType() == Type::kString) {
    return atoi(val_str);
  } else if (GetType() == Type::kFloat) {
    return (int)val_float;
  }
  return GetType() == Type::kInt ? val_int : 0;
}

float Value::GetFloat() const {
  if (GetType() == Type::kString) {
    return (float)atof(val_str);
  } else if (GetType() == Type::kInt) {
    return (float)val_int;
  }
  return GetType() == Type::kFloat ? val_float : 0;
}

const char* Value::GetString() {
  if (GetType() == Type::kInt) {
    char tmp[32];
    sprintf(tmp, "%d", val_int);
    SetString(tmp, Set::kNewCopy);
  } else if (GetType() == Type::kFloat) {
    char tmp[32];
    sprintf(tmp, "%f", val_float);
    SetString(tmp, Set::kNewCopy);
  } else if (GetType() == Type::kObject) {
    return val_obj ? val_obj->GetClassName() : "";
  }
  return GetType() == Type::kString ? val_str : "";
}

}  // namespace tb
