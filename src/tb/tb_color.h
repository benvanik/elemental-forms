/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_COLOR_H
#define TB_COLOR_H

#include <cstdint>

#include "tb_types.h"

namespace tb {

// Contains a 32bit color.
class Color {
 public:
  Color() = default;
  Color(int r, int g, int b, int a = 255) : b(b), g(g), r(r), a(a) {}

  uint8_t b = 0;
  uint8_t g = 0;
  uint8_t r = 0;
  uint8_t a = 0;

  void Set(const Color& color) { *this = color; }

  // Sets the color from string in any of the following formats:
  // "#rrggbbaa", "#rrggbb", "#rgba", "#rgb"
  void SetFromString(const char* str, size_t len);

  operator uint32_t() const { return *((uint32_t*)this); }
  bool operator==(const Color& c) const { return *this == (uint32_t)c; }
  bool operator!=(const Color& c) const { return !(*this == c); }
};

}  // namespace tb

#endif  // TB_COLOR_H
