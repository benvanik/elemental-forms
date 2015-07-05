/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/separator.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void Separator::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Separator, Value::Type::kNull, ElementZ::kTop);
}

Separator::Separator() {
  SetSkinBg(TBIDC("Separator"), InvokeInfo::kNoCallbacks);
  SetState(Element::State::kDisabled, true);
}

}  // namespace elements
}  // namespace tb
