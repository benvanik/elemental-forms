/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_COLOR_H_
#define EL_COLOR_H_

#include <cassert>
#include <cstdint>
#include <string>

namespace el {

// Contains a 32bit color.
class Color {
 public:
  Color() = default;
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
      : b(b), g(g), r(r), a(a) {}

  uint8_t b = 0;
  uint8_t g = 0;
  uint8_t r = 0;
  uint8_t a = 0;

  void reset() {
    b = g = r = 0;
    a = 255;
  }
  void reset(uint8_t new_r, uint8_t new_g, uint8_t new_b, uint8_t new_a = 255) {
    b = new_b;
    g = new_g;
    r = new_r;
    a = new_a;
  }
  void reset(const Color& value) { *this = value; }
  void reset(uint32_t value) { *reinterpret_cast<uint32_t*>(this) = value; }
  // Sets the color from string in any of the following formats:
  // "#rrggbbaa", "#rrggbb", "#rgba", "#rgb"
  void reset(const char* value, size_t length = std::string::npos);
  void reset(std::string value) { reset(value.c_str(), value.size()); }

  operator uint32_t() const { return *reinterpret_cast<const uint32_t*>(this); }
  bool operator==(const Color& c) const { return *this == (uint32_t)c; }
  bool operator!=(const Color& c) const { return !(*this == c); }
};
static_assert(sizeof(Color) == sizeof(uint32_t), "Treated as uint32_t");

}  // namespace el

#endif  // EL_COLOR_H_
