/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_hash.h"

namespace tb {

#ifndef TB_SUPPORT_CONSTEXPR

uint32 TBGetHash(const char* str) {
  if (!str || !*str) return 0;
  // FNV hash
  uint32 hash = 2166136261U;
  int i = 0;
  while (str[i]) {
    char c = str[i++];
    hash = (16777619U * hash) ^ c;
  }
  return hash;
}

#endif  // !TB_SUPPORT_CONSTEXPR

}  // namespace tb
