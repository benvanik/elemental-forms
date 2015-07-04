/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_BITMAP_FRAGMENT_MAP_H_
#define TB_RESOURCES_BITMAP_FRAGMENT_MAP_H_

#include <memory>
#include <vector>

#include "tb/resources/bitmap_fragment.h"

namespace tb {
namespace resources {

class BitmapFragmentManager;

// Used to pack multiple bitmaps into a single Bitmap.
// When initialized (in a size suitable for a Bitmap) is also creates a
// software buffer that will make up the Bitmap when all fragments have been
// added.
class BitmapFragmentMap {
 public:
  BitmapFragmentMap();
  ~BitmapFragmentMap();

  // Initializes the map with the given size.
  // The size should be a power of two since it will be used to create a
  // Bitmap (texture memory).
  bool Init(int bitmap_w, int bitmap_h);

  // Creates a new fragment with the given size and data in this map.
  // Returns nullptr if there is not enough room in this map or on any other
  // fail.
  BitmapFragment* CreateNewFragment(int frag_w, int frag_h, int data_stride,
                                    uint32_t* frag_data, bool add_border);

  // Frees up the space used by the given fragment, so that other fragments can
  // take its place.
  void FreeFragmentSpace(BitmapFragment* frag);

  // Returns the bitmap for this map.
  // By default, the bitmap is validated if needed before returning (See
  // Validate).
  Bitmap* GetBitmap(Validate validate_type = Validate::kAlways);

 private:
  friend class BitmapFragmentManager;
  bool ValidateBitmap();
  void DeleteBitmap();
  void CopyData(BitmapFragment* frag, int data_stride, uint32_t* frag_data,
                int border);

  std::vector<std::unique_ptr<BitmapFragmentSpaceAllocator>> m_rows;
  int m_bitmap_w = 0;
  int m_bitmap_h = 0;
  uint32_t* m_bitmap_data = nullptr;
  Bitmap* m_bitmap = nullptr;
  bool m_need_update = false;
  int m_allocated_pixels = 0;
};

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_BITMAP_FRAGMENT_MAP_H_
