/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TYPES_H_
#define TB_TYPES_H_

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstring>

#include "tb/config.h"

namespace tb {

#define MAKE_ORDERED_ENUM_STRING_UTILS(Enum, ...)                  \
  inline Enum from_string(const char* value, Enum default_value) { \
    static const char* text[] = {__VA_ARGS__};                     \
    for (int i = 0; i < sizeof(text) / sizeof(const char*); ++i) { \
      if (std::strcmp(value, text[i]) == 0) {                      \
        return static_cast<Enum>(i);                               \
      }                                                            \
    }                                                              \
    return default_value;                                          \
  }                                                                \
  inline const char* to_string(Enum value) {                       \
    static const char* text[] = {__VA_ARGS__};                     \
    return text[int(value)];                                       \
  \
}

// Makes it possible to use the given enum types as flag combinations.
// That will catch use of incorrect type during compilation, that wouldn't be
// caught using a uint32_t flag.
#define MAKE_ENUM_FLAG_COMBO(Enum)                       \
  constexpr bool any(const Enum value) {                 \
    return static_cast<uint32_t>(value) != 0;            \
  }                                                      \
  constexpr Enum operator|(Enum a, Enum b) {             \
    return static_cast<Enum>(static_cast<uint32_t>(a) |  \
                             static_cast<uint32_t>(b));  \
  }                                                      \
  constexpr Enum operator&(Enum a, Enum b) {             \
    return static_cast<Enum>(static_cast<uint32_t>(a) &  \
                             static_cast<uint32_t>(b));  \
  }                                                      \
  constexpr Enum operator^(Enum a, Enum b) {             \
    return static_cast<Enum>(static_cast<uint32_t>(a) ^  \
                             static_cast<uint32_t>(b));  \
  } \
inline void                                              \
  operator|=(Enum& a, Enum b) {                          \
    a = static_cast<Enum>(static_cast<uint32_t>(a) |     \
                          static_cast<uint32_t>(b));     \
  }                                                      \
  inline void operator&=(Enum& a, Enum b) {              \
    a = static_cast<Enum>(static_cast<uint32_t>(a) &     \
                          static_cast<uint32_t>(b));     \
  }                                                      \
  inline void operator^=(Enum& a, Enum b) {              \
    a = static_cast<Enum>(static_cast<uint32_t>(a) ^     \
                          static_cast<uint32_t>(b));     \
  }                                                      \
  inline Enum operator~(Enum a) {                        \
    return static_cast<Enum>(~static_cast<uint32_t>(a)); \
  }

}  // namespace tb

#endif  // TB_TYPES_H_
