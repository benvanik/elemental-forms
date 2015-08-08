/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdio>

#include "el/util/debug.h"
#include "el/util/string.h"

#define NOMINMAX
#include <windows.h>  // NOLINT(build/include_order)

#ifdef EL_RUNTIME_DEBUG_INFO

void TBDebugOut(const char* format, ...) {
  va_list va;
  va_start(va, format);
  auto result = el::util::format_string(format, va);
  va_end(va);
  OutputDebugStringA(result.c_str());
}

#endif  // EL_RUNTIME_DEBUG_INFO
