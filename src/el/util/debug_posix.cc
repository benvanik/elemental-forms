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

#ifdef EL_RUNTIME_DEBUG_INFO

void TBDebugOut(const char* str) { printf("%s", str); }

#endif  // EL_RUNTIME_DEBUG_INFO
