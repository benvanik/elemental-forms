/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/resizer.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void Resizer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Resizer, Value::Type::kNull, ElementZ::kTop);
}

Resizer::Resizer() {
  set_background_skin(TBIDC("Resizer"), InvokeInfo::kNoCallbacks);
}

HitStatus Resizer::GetHitStatus(int x, int y) {
  // Shave off some of the upper left diagonal half from the hit area.
  const int extra_hit_area = 3;
  if (x < rect().w - y - extra_hit_area) {
    return HitStatus::kNoHit;
  }
  return Element::GetHitStatus(x, y);
}

bool Resizer::OnEvent(const ElementEvent& ev) {
  Element* target = parent();
  if (!target) return false;
  if (ev.type == EventType::kPointerMove && captured_element == this) {
    int dx = ev.target_x - pointer_down_element_x;
    int dy = ev.target_y - pointer_down_element_y;
    Rect rect = target->rect();
    rect.w += dx;
    rect.h += dy;
    // Apply limit. We should not use minimum size since we can squeeze
    // the layout much more, and provide scroll/pan when smaller.
    rect.w = std::max(rect.w, 50);
    rect.h = std::max(rect.h, 50);
    target->set_rect(rect);
  } else {
    return false;
  }
  return true;
}

}  // namespace elements
}  // namespace tb
