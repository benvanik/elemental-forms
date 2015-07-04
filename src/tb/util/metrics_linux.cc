/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb/util/metrics.h"

#include <sys/time.h>

namespace tb {
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
}  // namespace tb
