/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include <cstdio>

#include "tb_color.h"

namespace tb {

void TBColor::SetFromString(const char* str, size_t len) {
  int r, g, b, a;
  if (len == 9 &&
      sscanf(str, "#%2x%2x%2x%2x", &r, &g, &b, &a) == 4)  // rrggbbaa
    Set(TBColor(r, g, b, a));
  else if (len == 7 && sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3)  // rrggbb
    Set(TBColor(r, g, b));
  else if (len == 5 &&
           sscanf(str, "#%1x%1x%1x%1x", &r, &g, &b, &a) == 4)  // rgba
    Set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4), a + (a << 4)));
  else if (len == 4 && sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3)  // rgb
    Set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4)));
  else
    Set(TBColor());
}

}  // namespace tb
