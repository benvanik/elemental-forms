/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_STRING_H_
#define EL_UTIL_STRING_H_

#include <cstdarg>
#include <cstring>
#include <string>

namespace el {
namespace util {

const char* stristr(const char* arg1, const char* arg2);

// Return true if the given string starts with a number.
// Ex: 100, -.2, 1.0E-8, 5px will all return true.
bool is_start_of_number(const char* str);

std::string format_string(const char* format, va_list args);
inline std::string format_string(const char* format, ...) {
  va_list va;
  va_start(va, format);
  auto result = format_string(format, va);
  va_end(va);
  return result;
}

bool is_space(int8_t c);
bool is_linebreak(int8_t c);
bool is_wordbreak(int8_t c);

}  // namespace util
}  // namespace el

#endif  // EL_UTIL_STRING_H_
