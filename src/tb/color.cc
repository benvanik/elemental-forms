/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include <cstdio>

#include "tb/color.h"

namespace tb {

void Color::reset(const char* value, size_t length) {
  if (!value || !length) {
    reset();
    return;
  }
  if (length == std::string::npos) {
    length = std::strlen(value);
  }
  int r, g, b, a;
  if (length == 9 && std::sscanf(value, "#%2x%2x%2x%2x", &r, &g, &b, &a) == 4) {
    // rrggbbaa
    reset(r, g, b, a);
  } else if (length == 7 && std::sscanf(value, "#%2x%2x%2x", &r, &g, &b) == 3) {
    // rrggbb
    reset(r, g, b);
  } else if (length == 5 &&
             std::sscanf(value, "#%1x%1x%1x%1x", &r, &g, &b, &a) == 4) {
    // rgba
    reset(r + (r << 4), g + (g << 4), b + (b << 4), a + (a << 4));
  } else if (length == 4 && std::sscanf(value, "#%1x%1x%1x", &r, &g, &b) == 3) {
    // rgb
    reset(r + (r << 4), g + (g << 4), b + (b << 4));
  } else {
    reset();
  }
}

}  // namespace tb
