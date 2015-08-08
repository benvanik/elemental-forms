/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdio>
#include <cstring>

#include "el/color.h"

namespace el {

void Color::reset(const char* value, size_t length) {
  if (!value || !length) {
    reset();
    return;
  }
  if (length == std::string::npos) {
    length = std::strlen(value);
  }
  int new_r, new_g, new_b, new_a;
  if (length == 9 &&
      std::sscanf(value, "#%2x%2x%2x%2x", &new_r, &new_g, &new_b, &new_a) ==
          4) {
    // rrggbbaa
    reset(new_r, new_g, new_b, new_a);
  } else if (length == 7 &&
             std::sscanf(value, "#%2x%2x%2x", &new_r, &new_g, &new_b) == 3) {
    // rrggbb
    reset(new_r, new_g, new_b);
  } else if (length == 5 &&
             std::sscanf(value, "#%1x%1x%1x%1x", &new_r, &new_g, &new_b,
                         &new_a) == 4) {
    // rgba
    reset(new_r + (new_r << 4), new_g + (new_g << 4), new_b + (new_b << 4),
          new_a + (new_a << 4));
  } else if (length == 4 &&
             std::sscanf(value, "#%1x%1x%1x", &new_r, &new_g, &new_b) == 3) {
    // rgb
    reset(new_r + (new_r << 4), new_g + (new_g << 4), new_b + (new_b << 4));
  } else {
    reset();
  }
}

}  // namespace el
