/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/graphics/bitmap_fragment.h"
#include "el/graphics/bitmap_fragment_map.h"

namespace el {
namespace graphics {

Bitmap* BitmapFragment::GetBitmap(Validate validate_type) {
  return m_map->GetBitmap(validate_type);
}

}  // namespace graphics
}  // namespace el
