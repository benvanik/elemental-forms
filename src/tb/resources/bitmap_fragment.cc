/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/resources/bitmap_fragment.h"
#include "tb/resources/bitmap_fragment_map.h"

namespace tb {
namespace resources {

Bitmap* BitmapFragment::GetBitmap(Validate validate_type) {
  return m_map->GetBitmap(validate_type);
}

}  // namespace resources
}  // namespace tb
