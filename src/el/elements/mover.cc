/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/mover.h"
#include "el/parsing/element_inflater.h"
#include "el/util/math.h"

namespace el {
namespace elements {

void Mover::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Mover, Value::Type::kNull, ElementZ::kTop);
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
}  // namespace el
