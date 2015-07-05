/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "tb/util/string.h"

namespace tb {
namespace util {

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

bool is_start_of_number(const char* str) {
  if (*str == '-') str++;
  if (*str == '.') str++;
  return *str >= '0' && *str <= '9';
}

std::string format_string(const char* format, va_list args) {
  if (!format) {
    return "";
  }
  size_t max_len = 64;
  std::string new_s;
  while (true) {
    new_s.resize(max_len);
    int ret = std::vsnprintf((char*)new_s.data(), max_len, format, args);
    if (ret > max_len) {
      // Needed size is known (+2 for termination and avoid ambiguity).
      max_len = ret + 2;
    } else if (ret == -1 || ret >= max_len - 1) {
      // Handle some buggy vsnprintf implementations.
      max_len *= 2;
    } else {
      // Everything fit for sure.
      return new_s;
    }
  }
}

bool is_space(int8_t c) {
  switch (c) {
    case ' ':
      return true;
  }
  return false;
}

bool is_linebreak(int8_t c) {
  switch (c) {
    case '\n':
    case '\r':
    case 0:
      return true;
  }
  return false;
}

bool is_wordbreak(int8_t c) {
  switch (c) {
    case 0:
    case '\n':
    case '\r':
    case '-':
    case '\t':
    case '\"':
    case '(':
    case ')':
    case '/':
    case '\\':
    case '*':
    case '+':
    case ',':
    case '.':
    case ';':
    case ':':
    case '>':
    case '<':
    case '&':
    case '#':
    case '!':
    case '=':
    case '[':
    case ']':
    case '{':
    case '}':
    case '^':
      return true;
  }
  return is_space(c);
}

}  // namespace util
}  // namespace tb
