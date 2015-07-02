/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_SKIN_UTIL_H
#define TB_SKIN_UTIL_H

#include "tb_skin.h"

namespace tb {

// Draw fade out skin elements at the edges of dst_rect if needed.
// It indicates to the user that there is hidden content.
// left, top, right, bottom specifies the (positive) distance scrolled from the
// limit.
void DrawEdgeFadeout(const Rect& dst_rect, TBID skin_x, TBID skin_y, int left,
                     int top, int right, int bottom);

}  // namespace tb

#endif  // TB_SKIN_UTIL_H
