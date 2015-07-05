/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "tb/parsing/element_inflater.h"
#include "tb/parts/mover.h"
#include "tb/util/math.h"

namespace tb {
namespace parts {

void Mover::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Mover, Value::Type::kNull, ElementZ::kTop);
}

Mover::Mover() { SetSkinBg(TBIDC("Mover"), InvokeInfo::kNoCallbacks); }

bool Mover::OnEvent(const ElementEvent& ev) {
  Element* target = GetParent();
  if (!target) return false;
  if (ev.type == EventType::kPointerMove && captured_element == this) {
    int dx = ev.target_x - pointer_down_element_x;
    int dy = ev.target_y - pointer_down_element_y;
    Rect rect = target->rect().Offset(dx, dy);
    if (target->GetParent()) {
      // Apply limit.
      rect.x =
          util::Clamp(rect.x, -pointer_down_element_x,
                      target->GetParent()->rect().w - pointer_down_element_x);
      rect.y =
          util::Clamp(rect.y, -pointer_down_element_y,
                      target->GetParent()->rect().h - pointer_down_element_y);
    }
    target->set_rect(rect);
    return true;
  }
  return false;
}

}  // namespace parts
}  // namespace tb
