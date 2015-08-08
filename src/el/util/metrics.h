/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_METRICS_H_
#define EL_UTIL_METRICS_H_

#include <string>

#include "el/config.h"

namespace el {
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
}  // namespace el

#endif  // EL_UTIL_METRICS_H_
