/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_TYPES_H
#define TB_TYPES_H

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <string>

#include "tb_config.h"

namespace tb {

template <class T>
T Clamp(const T& value, const T& min, const T& max) {
  return (value > max) ? max : ((value < min) ? min : value);
}

/** Returns value clamped to min and max. If max is greater than min,
        max will be clipped to min. */
template <class T>
T ClampClipMax(const T& value, const T& min, const T& max) {
  return (value > max) ? (max > min ? max : min)
                       : ((value < min) ? min : value);
}

inline const char* stristr(const char* arg1, const char* arg2) {
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

inline std::string format_string(const char* format, ...) {
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
    } else if (ret == -1 ||
               ret >= max_len -
                          1) {  // Handle some buggy vsnprintf implementations.
      max_len *= 2;
    } else {
      // Everything fit for sure
      return new_s;
    }
  }
}

/** Makes it possible to use the given enum types as flag combinations.
        That will catch use of incorrect type during compilation, that wouldn't
   be caught
        using a uint32_t flag. */
#define MAKE_ENUM_FLAG_COMBO(Enum)                       \
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

#endif  // TB_TYPES_H
