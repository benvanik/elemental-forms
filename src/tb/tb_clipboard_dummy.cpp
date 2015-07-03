/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_system.h"

#ifdef TB_CLIPBOARD_DUMMY

#include <sys/time.h>

#include <cstdio>

namespace tb {

std::string clipboard;

void TBClipboard::Empty() { clipboard.clear(); }

bool TBClipboard::HasText() { return !clipboard.empty(); }

bool TBClipboard::SetText(const std::string& text) {
  clipboard = text;
  return true;
}

std::string TBClipboard::GetText() { return clipboard; }

};  // namespace tb

#endif  // TB_CLIPBOARD_DUMMY
