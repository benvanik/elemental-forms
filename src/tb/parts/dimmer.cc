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
#include "tb/parts/dimmer.h"

namespace tb {
namespace parts {

void Dimmer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Dimmer, Value::Type::kNull, ElementZ::kTop);
}

Dimmer::Dimmer() {
  SetSkinBg(TBIDC("Dimmer"), InvokeInfo::kNoCallbacks);
  SetGravity(Gravity::kAll);
}

void Dimmer::OnAdded() {
  set_rect({0, 0, GetParent()->rect().w, GetParent()->rect().h});
}

}  // namespace parts
}  // namespace tb
