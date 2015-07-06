/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/mover.h"
#include "tb/parsing/element_inflater.h"
#include "tb/util/math.h"

namespace tb {
namespace elements {

void Mover::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Mover, Value::Type::kNull, ElementZ::kTop);
}

Mover::Mover() {
  set_background_skin(TBIDC("Mover"), InvokeInfo::kNoCallbacks);
}

bool Mover::OnEvent(const Event& ev) {
  Element* target = parent();
  if (!target) return false;
  if (ev.type == EventType::kPointerMove && captured_element == this) {
    int dx = ev.target_x - pointer_down_element_x;
    int dy = ev.target_y - pointer_down_element_y;
    Rect rect = target->rect().Offset(dx, dy);
    if (target->parent()) {
      // Apply limit.
      rect.x = util::Clamp(rect.x, -pointer_down_element_x,
                           target->parent()->rect().w - pointer_down_element_x);
      rect.y = util::Clamp(rect.y, -pointer_down_element_y,
                           target->parent()->rect().h - pointer_down_element_y);
    }
    target->set_rect(rect);
    return true;
  }
  return Element::OnEvent(ev);
}

}  // namespace elements
}  // namespace tb
