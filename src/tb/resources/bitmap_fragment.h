/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_BITMAP_FRAGMENT_H_
#define TB_RESOURCES_BITMAP_FRAGMENT_H_

#include <memory>
#include <vector>

#include "tb_id.h"

#include "tb/rect.h"
#include "tb/util/space_allocator.h"

namespace tb {
class Bitmap;
}  // namespace tb

namespace tb {
namespace resources {

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
  int Width() const { return m_rect.w; }
  int Height() const { return m_rect.h; }

  // Returns the bitmap for this fragment.
  // By default, the bitmap is validated if needed before returning (See
  // Validate).
  Bitmap* GetBitmap(Validate validate_type = Validate::kAlways);

  // Returns the height allocated to this fragment. This may be larger than
  // Height() depending of the internal allocation of fragments in a map. It
  // should rarely be used.
  int GetAllocatedHeight() const { return m_row_height; }

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

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_BITMAP_FRAGMENT_H_
