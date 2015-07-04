/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tb/util/object.h"
#include "tb/util/string.h"
#include "tb/value.h"

namespace tb {

// FIX: ## Floating point string conversions might be locale dependant. Force
// "." as decimal!

// Returns true if the given string contains space that is not at the end of the
// string.
bool contains_non_trailing_space(const char* str) {
  if (const char* p = strstr(str, " ")) {
    while (*p == ' ') p++;
    return *p != '\0';
  }
  return false;
}

// Return true if the string can be represented as a number.
// It ignores trailing white space.
// Ex: 100, -.2 will return true.
// Ex: 1.0E-8, 5px will return false.
bool is_number_only(const char* s) {
  if (!s || *s == 0 || *s == ' ') return 0;
  char* p;
  strtod(s, &p);
  while (*p == ' ') p++;
  return *p == '\0';
}

// Return true if the given number string is a float number.
// Should only be called when you've verified it's a number with is_number().
bool is_number_float(const char* str) {
  while (*str)
    if (*str++ == '.') return true;
  return false;
}

char* next_token(char*& str, const char* delim) {
  str += strspn(str, delim);
  if (!*str) return nullptr;
  char* token = str;
  str += strcspn(str, delim);
  if (*str) *str++ = '\0';
  return token;
}

ValueArray::ValueArray() = default;

ValueArray::~ValueArray() = default;

ValueArray* ValueArray::Clone(ValueArray* source) {
  ValueArray* new_arr = new ValueArray();
  for (auto& it : source->list_) {
    Value* new_val = new_arr->AddValue();
    new_val->Copy(*it.get());
  }
  return new_arr;
}

Value* ValueArray::AddValue() {
  list_.push_back(std::make_unique<Value>());
  return list_.back().get();
}

Value* ValueArray::AddInteger(int32_t value) {
  auto v = AddValue();
  v->set_integer(value);
  return v;
}

Value* ValueArray::at(size_t i) const {
  if (i >= 0 && i < list_.size()) {
    return list_[i].get();
  }
  return nullptr;
}

Value::Value() = default;

Value::Value(const Value& value) { Copy(value); }

Value::Value(Type type) {
  switch (type) {
    case Type::kNull:
      set_null();
      break;
    case Type::kString:
      set_string("", Set::kAsStatic);
      break;
    case Type::kFloat:
      set_float(0);
      break;
    case Type::kInt:
      set_integer(0);
      break;
    case Type::kObject:
      set_object(nullptr);
      break;
    case Type::kArray:
      set_array(new ValueArray(), Set::kTakeOwnership);
      break;
    default:
      assert(!"Not implemented!");
      break;
  };
}

Value::Value(int value) { set_integer(value); }

Value::Value(float value) { set_float(value); }

Value::Value(const char* value, Set set) { set_string(value, set); }

Value::Value(util::TypedObject* object) { set_object(object); }

Value::~Value() { set_null(); }

void Value::TakeOver(Value& source_value) {
  if (Type(source_value.packed_.type) == Type::kString) {
    set_string(source_value.value_.string, source_value.packed_.allocated
                                               ? Set::kTakeOwnership
                                               : Set::kNewCopy);
  } else if (source_value.type() == Type::kArray) {
    set_array(source_value.value_.value_array, source_value.packed_.allocated
                                                   ? Set::kTakeOwnership
                                                   : Set::kNewCopy);
  } else {
    *this = source_value;
  }
  source_value.set_type(Type::kNull);
}

void Value::Copy(const Value& source_value) {
  if (source_value.type() == Type::kString) {
    set_string(source_value.value_.string, Set::kNewCopy);
  } else if (source_value.type() == Type::kArray) {
    set_array(source_value.value_.value_array, Set::kNewCopy);
  } else if (source_value.type() == Type::kObject) {
    assert(!"We can't copy objects! The value will be nulled!");
    set_object(nullptr);
  } else {
    set_null();
    std::memcpy(this, &source_value, sizeof(Value));
  }
}

void Value::set_null() {
  if (packed_.allocated) {
    if (type() == Type::kString) {
      free(value_.string);
      value_.string = nullptr;
    } else if (type() == Type::kObject) {
      delete value_.object;
      value_.object = nullptr;
    } else if (type() == Type::kArray) {
      delete value_.value_array;
      value_.value_array = nullptr;
    }
  }
  set_type(Type::kNull);
}

void Value::set_integer(int val) {
  set_null();
  set_type(Type::kInt);
  value_.integer_number = val;
}

void Value::set_float(float val) {
  set_null();
  set_type(Type::kFloat);
  value_.float_number = val;
}

void Value::set_string(const char* val, Set set) {
  set_null();
  packed_.allocated = set == Set::kNewCopy || set == Set::kTakeOwnership;
  if (set != Set::kNewCopy) {
    value_.string = const_cast<char*>(val);
    set_type(Type::kString);
  } else if ((value_.string = strdup(val))) {
    set_type(Type::kString);
  }
}

void Value::set_object(util::TypedObject* object) {
  set_null();
  set_type(Type::kObject);
  packed_.allocated = true;
  value_.object = object;
}

void Value::set_array(ValueArray* arr, Set set) {
  set_null();
  packed_.allocated = set == Set::kNewCopy || set == Set::kTakeOwnership;
  if (set != Set::kNewCopy) {
    value_.value_array = arr;
    set_type(Type::kArray);
  } else if ((value_.value_array = ValueArray::Clone(arr))) {
    set_type(Type::kArray);
  }
}

void Value::parse_string(const char* str, Set set) {
  if (!str) {
    set_null();
  } else if (is_number_only(str)) {
    if (is_number_float(str)) {
      set_float((float)atof(str));
    } else {
      set_integer(atoi(str));
    }
  } else if (util::is_start_of_number(str) &&
             contains_non_trailing_space(str)) {
    // If the number has nontrailing space, we'll assume a list of numbers.
    // (example: "10 -4 3.5")
    set_null();
    ValueArray* arr = new ValueArray();
    std::string tmpstr = str;
    char* str_next = (char*)tmpstr.data();
    while (char* token = next_token(str_next, ", ")) {
      Value* new_val = arr->AddValue();
      new_val->parse_string(token, Set::kNewCopy);
    }
    set_array(arr, Set::kTakeOwnership);
  } else if (*str == '[') {
    set_null();
    ValueArray* arr = new ValueArray();
    assert(!"not implemented! Split out the tokenizer code above!");
    set_array(arr, Set::kTakeOwnership);
  } else {
    set_string(str, set);
    return;
  }
  // We didn't set as string, so we might need to deal with the passed in string
  // data.
  if (set == Set::kTakeOwnership) {
    // Delete the passed in data.
    Value tmp;
    tmp.set_string(str, Set::kTakeOwnership);
  }
}

int Value::as_integer() const {
  if (type() == Type::kString) {
    return atoi(value_.string);
  } else if (type() == Type::kFloat) {
    return (int)value_.float_number;
  }
  return type() == Type::kInt ? value_.integer_number : 0;
}

float Value::as_float() const {
  if (type() == Type::kString) {
    return (float)atof(value_.string);
  } else if (type() == Type::kInt) {
    return (float)value_.integer_number;
  }
  return type() == Type::kFloat ? value_.float_number : 0;
}

const char* Value::as_string() {
  if (type() == Type::kInt) {
    char tmp[32];
    sprintf(tmp, "%d", value_.integer_number);
    set_string(tmp, Set::kNewCopy);
  } else if (type() == Type::kFloat) {
    char tmp[32];
    sprintf(tmp, "%f", value_.float_number);
    set_string(tmp, Set::kNewCopy);
  } else if (type() == Type::kObject) {
    return value_.object ? value_.object->GetClassName() : "";
  }
  return type() == Type::kString ? value_.string : "";
}

std::string Value::to_string() const {
  switch (type()) {
    case Type::kNull:
      return "";
    case Type::kString:
      return value_.string;
    case Type::kFloat:
      return std::to_string(value_.float_number);
    case Type::kInt:
      return std::to_string(value_.integer_number);
    case Type::kObject:
      return value_.object ? value_.object->GetClassName() : "";
    case Type::kArray: {
      std::string result;
      for (size_t i = 0; i < value_.value_array->size(); ++i) {
        auto value = value_.value_array->at(i);
        if (i) {
          result += " " + value->to_string();
        } else {
          result += value->to_string();
        }
      }
      return result;
    }
    default:
      return "";
  }
}

}  // namespace tb
