/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cassert>

#include "el/util/metrics.h"

#define NOMINMAX
#include <windows.h>

namespace el {
namespace util {

uint64_t GetTimeMS() {
  FILETIME t;
  GetSystemTimeAsFileTime(&t);
  return ((uint64_t(t.dwHighDateTime) << 32) | t.dwLowDateTime) / 10000;
}

int GetLongClickDelayMS() { return 500; }

int GetPanThreshold() { return 5 * GetDPI() / 96; }

int GetPixelsPerLine() { return 40 * GetDPI() / 96; }

int GetDPI() {
  HDC hdc = GetDC(nullptr);
  assert(hdc != nullptr);
  int dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
  ReleaseDC(nullptr, hdc);
#if 0  // TEST CODE!
  dpi_x *= 2;
#endif
  return dpi_x;
}

}  // namespace util
}  // namespace el
