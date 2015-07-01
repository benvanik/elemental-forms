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

TBStr clipboard;  ///< Obviosly not a full implementation since it ignores the
/// OS :)

void TBClipboard::Empty() { clipboard.Clear(); }

bool TBClipboard::HasText() { return !clipboard.IsEmpty(); }

bool TBClipboard::SetText(const char* text) { return clipboard.Set(text); }

TBStr TBClipboard::GetText() { return clipboard; }

};  // namespace tb

#endif  // TB_CLIPBOARD_DUMMY
