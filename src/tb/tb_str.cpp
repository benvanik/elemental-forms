/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_str.h"

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace tb {

static const char* kEmptyString = "";
inline void safe_delete(char*& str) {
  if (str != kEmptyString && str) free(str);
  str = const_cast<char*>(kEmptyString);
}

TBStr::TBStr() : s(const_cast<char*>(kEmptyString)) {}

TBStr::TBStr(const char* str)
    : s(const_cast<char*>(str == kEmptyString ? kEmptyString : strdup(str))) {
  if (!s) {
    s = const_cast<char*>(kEmptyString);
  }
}

TBStr::TBStr(const TBStr& str)
    : s(const_cast<char*>(str.s == kEmptyString ? kEmptyString
                                                : strdup(str.s))) {
  if (!s) {
    s = const_cast<char*>(kEmptyString);
  }
}

TBStr::TBStr(const char* str, size_t len) : s(const_cast<char*>(kEmptyString)) {
  assign(str, len);
}

TBStr::~TBStr() { safe_delete(s); }

void TBStr::assign(const char* str, size_t len) {
  safe_delete(s);
  if (len == std::string::npos) len = strlen(str);
  char* new_s = (char*)malloc(len + 1);
  s = new_s;
  memcpy(s, str, len);
  s[len] = 0;
}

void TBStr::clear() { safe_delete(s); }

void TBStr::erase(size_t ofs, size_t len) {
  assert(ofs >= 0 && (ofs + len <= (int)strlen(s)));
  if (!len) return;
  char* dst = s + ofs;
  char* src = s + ofs + len;
  while (*src != 0) *(dst++) = *(src++);
  *dst = *src;
}

void TBStr::insert(size_t ofs, const char* ins, size_t ins_len) {
  size_t len1 = strlen(s);
  if (ins_len == std::string::npos) {
    ins_len = strlen(ins);
  }
  size_t newlen = len1 + ins_len;
  char* news = (char*)malloc(newlen + 1);
  memcpy(&news[0], s, ofs);
  memcpy(&news[ofs], ins, ins_len);
  memcpy(&news[ofs + ins_len], &s[ofs], len1 - ofs);
  news[newlen] = 0;
  safe_delete(s);
  s = news;
}

const char* stristr(const char* arg1, const char* arg2) {
  const char* a, *b;
  for (; *arg1; arg1++) {
    a = arg1;
    b = arg2;
    while (std::toupper(*a++) == std::toupper(*b++)) {
      if (!*b) return arg1;
    }
  }
  return nullptr;
}

TBStr format_string(const char* format, ...) {
  if (!format) return TBStr();
  va_list ap;
  size_t max_len = 64;
  char* new_s = nullptr;
  while (true) {
    char* tris_try_new_s = (char*)realloc(new_s, max_len);
    new_s = tris_try_new_s;

    va_start(ap, format);
    int ret = vsnprintf(new_s, max_len, format, ap);
    va_end(ap);

    if (ret > max_len) {
      // Needed size is known (+2 for termination and avoid
      // ambiguity)
      max_len = ret + 2;
    } else if (ret == -1 || ret >= max_len - 1) {
      // Handle some buggy vsnprintf implementations.
      max_len *= 2;
    } else {
      // Everything fit for sure
      return TBStr(new_s);
    }
  }
  return false;
}

/*
std::string format_string(const char* format, ...) {
  if (!format) return "";
  va_list ap;
  size_t max_len = std::min(size_t(64), std::strlen(format));
  std::string new_s;
  while (true) {
    new_s.resize(max_len);
    va_start(ap, format);
    int ret = vsnprintf((char*)new_s.data(), max_len, format, ap);
    va_end(ap);
    if (ret > max_len) {  // Needed size is known (+2 for termination and
                          // avoid ambiguity)
      max_len = ret + 2;
    }
    else if (ret == -1 ||
      ret >= max_len -
      1) {  // Handle some buggy vsnprintf implementations.
      max_len *= 2;
    }
    else {
      // Everything fit for sure
      return new_s;
    }
  }
}
*/

}  // namespace tb
