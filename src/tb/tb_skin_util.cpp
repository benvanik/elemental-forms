/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_skin_util.h"

namespace tb {

int GetFadeoutSize(int scrolled_distance, int fadeout_length) {
  // Make it appear gradually.
  // float factor = scrolled_distance / 10.f;
  // factor = Clamp(factor, 0.5f, 1);
  // return (int)(fadeout_length * factor);
  return scrolled_distance > 0 ? fadeout_length : 0;
}

void DrawEdgeFadeout(const Rect& dst_rect, TBID skin_x, TBID skin_y, int left,
                     int top, int right, int bottom) {
  if (SkinElement* skin = Skin::get()->GetSkinElement(skin_x)) {
    if (skin->bitmap) {
      int bw = skin->bitmap->Width();
      int bh = skin->bitmap->Height();
      int dw;
      if ((dw = GetFadeoutSize(left, bw)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x, dst_rect.y, dw, dst_rect.h), Rect(0, 0, bw, bh),
            skin->bitmap);
      }
      if ((dw = GetFadeoutSize(right, bw)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x + dst_rect.w - dw, dst_rect.y, dw, dst_rect.h),
            Rect(bw, 0, -bw, bh), skin->bitmap);
      }
    }
  }
  if (SkinElement* skin = Skin::get()->GetSkinElement(skin_y)) {
    if (skin->bitmap) {
      int bw = skin->bitmap->Width();
      int bh = skin->bitmap->Height();
      int dh;
      if ((dh = GetFadeoutSize(top, bh)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x, dst_rect.y, dst_rect.w, dh), Rect(0, 0, bw, bh),
            skin->bitmap);
      }
      if ((dh = GetFadeoutSize(bottom, bh)) > 0) {
        Renderer::get()->DrawBitmap(
            Rect(dst_rect.x, dst_rect.y + dst_rect.h - dh, dst_rect.w, dh),
            Rect(0, bh, bw, -bh), skin->bitmap);
      }
    }
  }
}

}  // namespace tb
