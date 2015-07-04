/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_STRING_H_
#define TB_UTIL_STRING_H_

#include <cstdarg>
#include <cstring>
#include <string>

namespace tb {
namespace util {

const char* stristr(const char* arg1, const char* arg2);

std::string format_string(const char* format, va_list args);
inline std::string format_string(const char* format, ...) {
  va_list va;
  va_start(va, format);
  auto result = format_string(format, va);
  va_end(va);
  return result;
}

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_STRING_H_
