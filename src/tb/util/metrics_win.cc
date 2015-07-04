/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include <cassert>

#include "tb/util/metrics.h"

#define NOMINMAX
#include <windows.h>

namespace tb {
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
}  // namespace tb
