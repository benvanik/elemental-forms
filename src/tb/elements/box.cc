/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/box.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void Box::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Box, Value::Type::kNull, ElementZ::kTop);
}

Box::Box() { SetSkinBg(TBIDC("Box"), InvokeInfo::kNoCallbacks); }

}  // namespace elements
}  // namespace tb
