/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb/elements/skin_image.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void SkinImage::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(SkinImage, Value::Type::kNull, ElementZ::kTop);
}

PreferredSize SkinImage::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredSize(constraints);
  // FIX: Make it stretched proportionally if shrunk.
  ps.max_w = ps.pref_w;
  ps.max_h = ps.pref_h;
  return ps;
}

}  // namespace elements
}  // namespace tb
