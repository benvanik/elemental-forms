/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include <cstdio>

#include "tb/util/debug.h"

#ifdef TB_RUNTIME_DEBUG_INFO

void TBDebugOut(const char* str) { printf("%s", str); }

#endif  // TB_RUNTIME_DEBUG_INFO
