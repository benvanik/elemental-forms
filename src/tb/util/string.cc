/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
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

}  // namespace util
}  // namespace tb
