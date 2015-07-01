/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_STR_H
#define TB_STR_H

#include <cstring>
#include <string>

#include "tb_types.h"

namespace tb {

const char* stristr(const char* arg1, const char* arg2);

std::string format_string(const char* format, ...);

}  // namespace tb

#endif  // TB_STR_H
