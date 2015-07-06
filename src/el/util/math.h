/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_MATH_H_
#define EL_UTIL_MATH_H_

#include <algorithm>
#include <cstdint>

namespace el {
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
}  // namespace el

#endif  // EL_UTIL_MATH_H_
