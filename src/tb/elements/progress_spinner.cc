/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/progress_spinner.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void ProgressSpinner::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(ProgressSpinner, Value::Type::kInt,
                               ElementZ::kTop);
}

ProgressSpinner::ProgressSpinner() {
  set_background_skin(TBIDC("ProgressSpinner"), InvokeInfo::kNoCallbacks);
  m_skin_fg.reset(TBIDC("ProgressSpinner.fg"));
}

void ProgressSpinner::set_value(int value) {
  if (value == m_value) return;
  InvalidateSkinStates();
  assert(value >=
         0);  // If this happens, you probably have unballanced Begin/End calls.
  m_value = value;
  if (value > 0) {
    // Start animation.
    if (!GetMessageById(TBID(1))) {
      m_frame = 0;
      PostMessageDelayed(TBID(1), nullptr, kSpinSpeed);
    }
  } else {
    // Stop animation.
    if (Message* msg = GetMessageById(TBID(1))) {
      DeleteMessage(msg);
    }
  }
}

void ProgressSpinner::OnPaint(const PaintProps& paint_props) {
  if (is_animating()) {
    auto e = Skin::get()->GetSkinElementById(m_skin_fg);
    if (e && e->bitmap) {
      int size = e->bitmap->height();
      int num_frames = e->bitmap->width() / e->bitmap->height();
      int current_frame = m_frame % num_frames;
      graphics::Renderer::get()->DrawBitmap(
          padding_rect(), Rect(current_frame * size, 0, size, size), e->bitmap);
    }
  }
}

void ProgressSpinner::OnMessageReceived(Message* msg) {
  m_frame++;
  Invalidate();
  // Keep animation running.
  PostMessageDelayed(TBID(1), nullptr, kSpinSpeed);
}

}  // namespace elements
}  // namespace tb
