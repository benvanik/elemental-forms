/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>

#include "el/elements/resizer.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void Resizer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Resizer, Value::Type::kNull, ElementZ::kTop);
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

bool Resizer::OnEvent(const Event& ev) {
  Element* target = parent();
  if (!target) {
    return Element::OnEvent(ev);
  }
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
    return true;
  } else {
    return Element::OnEvent(ev);
  }
}

}  // namespace elements
}  // namespace el
