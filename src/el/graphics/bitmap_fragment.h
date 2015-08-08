/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_BITMAP_FRAGMENT_H_
#define EL_GRAPHICS_BITMAP_FRAGMENT_H_

#include <climits>
#include <memory>
#include <vector>

#include "el/id.h"
#include "el/rect.h"
#include "el/util/space_allocator.h"

namespace el {
namespace graphics {

class Bitmap;
class BitmapFragmentMap;

// Specify when the bitmap should be validated when calling
// BitmapFragmentMap::GetBitmap.
enum class Validate {
  // Always validate the bitmap (the bitmap is updated if needed).
  kAlways,
  // Only validate if the bitmap does not yet exist (make sure there is a valid
  // bitmap pointer, but the data is not necessarily updated).
  kFirstTime,
};

// Allocates space for BitmapFragment in a row (used in BitmapFragmentMap).
class BitmapFragmentSpaceAllocator : public util::SpaceAllocator {
 public:
  BitmapFragmentSpaceAllocator(int y, int width, int height)
      : util::SpaceAllocator(width), y(y), height(height) {}
  int y;
  int height;
};

// Represents a sub part of a Bitmap.
// It's owned by BitmapFragmentManager which pack multiple BitmapFragment within
// Bitmaps to reduce texture switching.
class BitmapFragment {
 public:
  int width() const { return m_rect.w; }
  int height() const { return m_rect.h; }

  // Returns the bitmap for this fragment.
  // By default, the bitmap is validated if needed before returning (See
  // Validate).
  graphics::Bitmap* GetBitmap(Validate validate_type = Validate::kAlways);

  // Returns the height allocated to this fragment. This may be larger than
  // Height() depending of the internal allocation of fragments in a map. It
  // should rarely be used.
  int allocated_height() const { return m_row_height; }

 public:
  BitmapFragmentMap* m_map = nullptr;
  Rect m_rect;
  BitmapFragmentSpaceAllocator* m_row = nullptr;
  BitmapFragmentSpaceAllocator::Space* m_space = nullptr;
  TBID m_id;
  int m_row_height = 0;

  // Reserved for batching renderer backends. It's not used internally, but
  // always initialized to 0xffffffff for all new fragments.
  uint32_t m_batch_id = UINT_MAX;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_BITMAP_FRAGMENT_H_
