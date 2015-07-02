/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_value.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tb_object.h"
#include "tb_str.h"

namespace tb {

// FIX: ## Floating point string conversions might be locale dependant. Force
// "." as decimal!

// == Helper functions ============================

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

TBValueArray::TBValueArray() {}

TBValueArray::~TBValueArray() {}

TBValue* TBValueArray::AddValue() {
  TBValue* v;
  if ((v = new TBValue()) && m_list.Add(v)) return v;
  delete v;
  return nullptr;
}

TBValue* TBValueArray::GetValue(int index) {
  if (index >= 0 && index < m_list.GetNumItems()) return m_list[index];
  return nullptr;
}

TBValueArray* TBValueArray::Clone(TBValueArray* source) {
  TBValueArray* new_arr = new TBValueArray;
  if (!new_arr) return nullptr;
  for (int i = 0; i < source->m_list.GetNumItems(); i++) {
    TBValue* new_val = new_arr->AddValue();
    if (!new_val) {
      delete new_arr;
      return nullptr;
    }
    new_val->Copy(*source->GetValue(i));
  }
  return new_arr;
}

TBValue::TBValue() : m_packed_init(0) {}

TBValue::TBValue(const TBValue& value) : m_packed_init(0) { Copy(value); }

TBValue::TBValue(Type type) : m_packed_init(0) {
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
      if (TBValueArray* arr = new TBValueArray())
        SetArray(arr, Set::kTakeOwnership);
      break;
    default:
      assert(!"Not implemented!");
  };
}

TBValue::TBValue(int value) : m_packed_init(0) { SetInt(value); }

TBValue::TBValue(float value) : m_packed_init(0) { SetFloat(value); }

TBValue::TBValue(const char* value, Set set) : m_packed_init(0) {
  SetString(value, set);
}

TBValue::TBValue(TBTypedObject* object) : m_packed_init(0) {
  SetObject(object);
}

TBValue::~TBValue() { SetNull(); }

void TBValue::TakeOver(TBValue& source_value) {
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

void TBValue::Copy(const TBValue& source_value) {
  if (source_value.GetType() == Type::kString) {
    SetString(source_value.val_str, Set::kNewCopy);
  } else if (source_value.GetType() == Type::kArray) {
    SetArray(source_value.val_arr, Set::kNewCopy);
  } else if (source_value.GetType() == Type::kObject) {
    assert(!"We can't copy objects! The value will be nulled!");
    SetObject(nullptr);
  } else {
    SetNull();
    memcpy(this, &source_value, sizeof(TBValue));
  }
}

void TBValue::SetNull() {
  if (m_packed.allocated) {
    if (GetType() == Type::kString) {
      free(val_str);
    } else if (GetType() == Type::kObject) {
      delete val_obj;
    } else if (GetType() == Type::kArray) {
      delete val_arr;
    }
  }
  SetType(Type::kNull);
}

void TBValue::SetInt(int val) {
  SetNull();
  SetType(Type::kInt);
  val_int = val;
}

void TBValue::SetFloat(float val) {
  SetNull();
  SetType(Type::kFloat);
  val_float = val;
}

void TBValue::SetString(const char* val, Set set) {
  SetNull();
  m_packed.allocated = (set == Set::kNewCopy || set == Set::kTakeOwnership);
  if (set != Set::kNewCopy) {
    val_str = const_cast<char*>(val);
    SetType(Type::kString);
  } else if ((val_str = strdup(val))) {
    SetType(Type::kString);
  }
}

void TBValue::SetObject(TBTypedObject* object) {
  SetNull();
  SetType(Type::kObject);
  m_packed.allocated = true;
  val_obj = object;
}

void TBValue::SetArray(TBValueArray* arr, Set set) {
  SetNull();
  m_packed.allocated = (set == Set::kNewCopy || set == Set::kTakeOwnership);
  if (set != Set::kNewCopy) {
    val_arr = arr;
    SetType(Type::kArray);
  } else if ((val_arr = TBValueArray::Clone(arr))) {
    SetType(Type::kArray);
  }
}

void TBValue::SetFromStringAuto(const char* str, Set set) {
  if (!str) {
    SetNull();
  } else if (is_number_only(str)) {
    if (is_number_float(str)) {
      SetFloat((float)atof(str));
    } else {
      SetInt(atoi(str));
    }
  } else if (is_start_of_number(str) && contains_non_trailing_space(str)) {
    // If the number has nontrailing space, we'll assume a list of numbers
    // (example: "10 -4 3.5")
    SetNull();
    if (TBValueArray* arr = new TBValueArray) {
      std::string tmpstr = str;
      char* str_next = (char*)tmpstr.data();
      while (char* token = next_token(str_next, ", ")) {
        if (TBValue* new_val = arr->AddValue()) {
          new_val->SetFromStringAuto(token, Set::kNewCopy);
        }
      }
      SetArray(arr, Set::kTakeOwnership);
    }
  } else if (*str == '[') {
    SetNull();
    if (TBValueArray* arr = new TBValueArray) {
      assert(!"not implemented! Split out the tokenizer code above!");
      SetArray(arr, Set::kTakeOwnership);
    }
  } else {
    SetString(str, set);
    return;
  }
  // We didn't set as string, so we might need to deal with the passed in string
  // data.
  if (set == Set::kTakeOwnership) {
    // Delete the passed in data
    TBValue tmp;
    tmp.SetString(str, Set::kTakeOwnership);
  }
}

int TBValue::GetInt() const {
  if (GetType() == Type::kString) {
    return atoi(val_str);
  } else if (GetType() == Type::kFloat) {
    return (int)val_float;
  }
  return GetType() == Type::kInt ? val_int : 0;
}

float TBValue::GetFloat() const {
  if (GetType() == Type::kString) {
    return (float)atof(val_str);
  } else if (GetType() == Type::kInt) {
    return (float)val_int;
  }
  return GetType() == Type::kFloat ? val_float : 0;
}

const char* TBValue::GetString() {
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
