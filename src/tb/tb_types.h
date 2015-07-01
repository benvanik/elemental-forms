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

#include <cstring>

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

/** Makes it possible to use the given enum types as flag combinations.
        That will catch use of incorrect type during compilation, that wouldn't
   be caught
        using a uint32_t flag. */
#define MAKE_ENUM_FLAG_COMBO(Enum)                       \
  inline Enum operator|(Enum a, Enum b) {                \
    return static_cast<Enum>(static_cast<uint32_t>(a) |  \
                             static_cast<uint32_t>(b));  \
  }                                                      \
  inline Enum operator&(Enum a, Enum b) {                \
    return static_cast<Enum>(static_cast<uint32_t>(a) &  \
                             static_cast<uint32_t>(b));  \
  }                                                      \
  inline Enum operator^(Enum a, Enum b) {                \
    return static_cast<Enum>(static_cast<uint32_t>(a) ^  \
                             static_cast<uint32_t>(b));  \
  } inline void                                          \
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
