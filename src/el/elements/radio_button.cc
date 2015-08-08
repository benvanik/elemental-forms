/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/radio_button.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void RadioButton::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(RadioButton, Value::Type::kInt, ElementZ::kTop);
}

}  // namespace elements
}  // namespace el
