/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/icon_box.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void IconBox::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(IconBox, Value::Type::kNull, ElementZ::kTop);
}

PreferredSize IconBox::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredSize(constraints);
  // FIX: Make it stretched proportionally if shrunk.
  ps.max_w = ps.pref_w;
  ps.max_h = ps.pref_h;
  return ps;
}

}  // namespace elements
}  // namespace el
