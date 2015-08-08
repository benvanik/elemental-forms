/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/util/metrics.h"

#include <sys/time.h>

namespace el {
namespace util {

uint64_t GetTimeMS() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_usec / 1000 + now.tv_sec * 1000;
}

int GetLongClickDelayMS() { return 500; }

int GetPanThreshold() { return 5 * GetDPI() / 96; }

int GetPixelsPerLine() { return 40 * GetDPI() / 96; }

int GetDPI() {
  // FIX: Implement!
  return 96;
}

}  // namespace util
}  // namespace el
