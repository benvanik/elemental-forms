/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/check_box.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void CheckBox::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(CheckBox, Value::Type::kInt, ElementZ::kTop);
}

}  // namespace elements
}  // namespace tb
