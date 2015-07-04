/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_UTIL_METRICS_H_
#define TB_UTIL_METRICS_H_

#include <string>

#include "tb/config.h"

namespace tb {
namespace util {

// Gets the system time in milliseconds since some undefined epoch.
uint64_t GetTimeMS();

// Gets how many milliseconds it should take after a touch down event should
// generate a long click event.
int GetLongClickDelayMS();

// Gets how many pixels of dragging should start panning scrollable elements.
int GetPanThreshold();

// Gets how many pixels a typical line is: The length that should be scrolled
// when turning a mouse wheel one notch.
int GetPixelsPerLine();

// Gets Dots Per Inch for the main screen.
int GetDPI();

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_METRICS_H_
