/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_HASH_H_
#define EL_UTIL_HASH_H_

#include <cstdint>

#include "el/config.h"

namespace el {
namespace util {

// On C++ compilers that support it, use const expr for hash so that
// TBID comparisions turn into simple uint32_t comparisions compiletime.
// Disabled for EL_RUNTIME_DEBUG_INFO builds, so TBID string debugging
// is available.
//
// Note: GCC may need -std=c++0x or -std=c++11 to enable this feature.

#ifndef EL_RUNTIME_DEBUG_INFO
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#define EL_SUPPORT_CONSTEXPR
#endif
#endif  // EL_RUNTIME_DEBUG_INFO

#ifdef EL_SUPPORT_CONSTEXPR

// FNV constants
static constexpr uint32_t basis = 2166136261U;
static constexpr uint32_t prime = 16777619U;

// compile-time hash helper function
constexpr uint32_t TBGetHash_one(char c, const char* remain, uint32_t value) {
  return c == 0 ? value
                : TBGetHash_one(remain[0], remain + 1, (value ^ c) * prime);
}

// compile-time hash
constexpr uint32_t hash(const char* str) {
  return (str && *str) ? TBGetHash_one(str[0], str + 1, basis) : 0;
}

#define TBIDC(str) el::util::hash(str)

#else  // EL_SUPPORT_CONSTEXPR

#define TBIDC(str) el::TBID(str)

// Gets a hash value from string.
inline uint32_t hash(const char* str) {
  if (!str || !*str) return 0;
  // FNV hash
  uint32_t hash = 2166136261U;
  int i = 0;
  while (str[i]) {
    char c = str[i++];
    hash = (16777619U * hash) ^ c;
  }
  return hash;
}

#endif  // !EL_SUPPORT_CONSTEXPR

}  // namespace util
}  // namespace el

#endif  // EL_UTIL_HASH_H_
