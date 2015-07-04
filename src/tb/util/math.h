/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_MATH_H_
#define TB_UTIL_MATH_H_

#include <algorithm>
#include <cstdint>

namespace tb {
namespace util {

// Returns the nearest power of two from val.
// F.ex 110 -> 128, 256->256, 257->512 etc.
inline int GetNearestPowerOfTwo(int val) {
  int i;
  for (i = 31; i >= 0; i--) {
    if ((val - 1) & (1 << i)) {
      break;
    }
  }
  return (1 << (i + 1));
}

template <class T>
T Clamp(const T& value, const T& min, const T& max) {
  return (value > max) ? max : ((value < min) ? min : value);
}

// Returns value clamped to min and max. If max is greater than min, max will be
// clipped to min.
template <class T>
T ClampClipMax(const T& value, const T& min, const T& max) {
  return (value > max) ? (max > min ? max : min)
                       : ((value < min) ? min : value);
}

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_MATH_H_
