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
#include "tb/parts/container.h"

namespace tb {
namespace parts {

void Container::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Container, Value::Type::kNull, ElementZ::kTop);
}

Container::Container() {
  SetSkinBg(TBIDC("Container"), InvokeInfo::kNoCallbacks);
}

}  // namespace parts
}  // namespace tb
