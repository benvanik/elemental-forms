/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/dimmer.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void Dimmer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Dimmer, Value::Type::kNull, ElementZ::kTop);
}

Dimmer::Dimmer() {
  set_background_skin(TBIDC("Dimmer"), InvokeInfo::kNoCallbacks);
  set_gravity(Gravity::kAll);
}

void Dimmer::OnAdded() {
  set_rect({0, 0, parent()->rect().w, parent()->rect().h});
}

}  // namespace elements
}  // namespace el
