/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/graphics/bitmap_fragment.h"
#include "tb/graphics/bitmap_fragment_map.h"

namespace tb {
namespace graphics {

Bitmap* BitmapFragment::GetBitmap(Validate validate_type) {
  return m_map->GetBitmap(validate_type);
}

}  // namespace graphics
}  // namespace tb
