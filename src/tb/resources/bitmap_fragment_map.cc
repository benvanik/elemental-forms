/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_renderer.h"

#include "tb/resources/bitmap_fragment_map.h"
#include "tb/util/math.h"
#include "tb/util/space_allocator.h"

namespace tb {
namespace resources {

BitmapFragmentMap::BitmapFragmentMap() = default;

bool BitmapFragmentMap::Init(int bitmap_w, int bitmap_h) {
  m_bitmap_data = new uint32_t[bitmap_w * bitmap_h];
  m_bitmap_w = bitmap_w;
  m_bitmap_h = bitmap_h;
#ifdef TB_RUNTIME_DEBUG_INFO
  std::memset(m_bitmap_data, 0x88, bitmap_w * bitmap_h * sizeof(uint32_t));
#endif  // TB_RUNTIME_DEBUG_INFO
  return m_bitmap_data ? true : false;
}

BitmapFragmentMap::~BitmapFragmentMap() {
  delete m_bitmap;
  delete[] m_bitmap_data;
}

BitmapFragment* BitmapFragmentMap::CreateNewFragment(int frag_w, int frag_h,
                                                     int data_stride,
                                                     uint32_t* frag_data,
                                                     bool add_border) {
  // Finding available space works like this:
  // The map size is sliced up horizontally in rows (initially just one row
  // covering
  // the entire map). When adding a new fragment, put it in the row with
  // smallest height.
  // If the smallest row is empty, it may slice the row to make a even smaller
  // row.

  // When a image is stretched up to a larger size, the filtering will read
  // pixels closest (but outside) of the src_rect. When we pack images together
  // those pixels would be read from neighbour images, so we must add border
  // space
  // around each image to avoid artifacts. We must also fill in that border with
  // the "clamp" of the image itself so we don't get any filtering artifacts at
  // all.
  // Always add border except when we're using the entire map for one fragment.
  int border = 0;
  int needed_w = frag_w;
  int needed_h = frag_h;
  if (add_border) {
    if (needed_w != m_bitmap_w || needed_h != m_bitmap_h) {
      border = 1;
      needed_w += 2;
      needed_h += 2;
    }
  }

  // Snap the fragments to a certain granularity. This could maybe ease the
  // stress
  // on the space allocator when allocating & deallocating lots of small
  // fragments.
  // I'm not sure there is any performance issue though and it would be better
  // to
  // optimize the algorithm instead (so disabled it for now).
  // const int granularity = 8;
  // needed_w = (needed_w + granularity - 1) / granularity * granularity;
  // needed_h = (needed_h + granularity - 1) / granularity * granularity;
  if (m_rows.empty()) {
    // Create a row covering the entire bitmap.
    auto row = std::make_unique<BitmapFragmentSpaceAllocator>(0, m_bitmap_w,
                                                              m_bitmap_h);
    m_rows.push_back(std::move(row));
  }
  // Get the smallest row where we fit.
  int best_row_index = -1;
  BitmapFragmentSpaceAllocator* best_row = nullptr;
  for (int i = 0; i < m_rows.size(); ++i) {
    auto row = m_rows[i].get();
    if (!best_row || row->height < best_row->height) {
      // This is the best row so far, if we fit.
      if (needed_h <= row->height && row->HasSpace(needed_w)) {
        best_row = row;
        best_row_index = i;
        if (needed_h == row->height) {
          break;  // We can't find a smaller line, so we're done.
        }
      }
    }
  }
  // Return if we're full.
  if (!best_row) {
    return nullptr;
  }
  // If the row is unused, create a smaller row to only consume needed height
  // for fragment.
  if (best_row->IsAllAvailable() && needed_h < best_row->height) {
    auto row = std::make_unique<BitmapFragmentSpaceAllocator>(
        best_row->y + needed_h, m_bitmap_w, best_row->height - needed_h);
    // Keep the rows sorted from top to bottom.
    m_rows.insert(m_rows.begin() + best_row_index + 1, std::move(row));
    best_row->height = needed_h;
  }
  // Allocate the fragment and copy the fragment data into the map data.
  if (auto space = best_row->AllocSpace(needed_w)) {
    BitmapFragment* frag = new BitmapFragment();
    frag->m_map = this;
    frag->m_row = best_row;
    frag->m_space = space;
    frag->m_rect.reset(space->x + border, best_row->y + border, frag_w, frag_h);
    frag->m_row_height = best_row->height;
    frag->m_batch_id = 0xffffffff;
    CopyData(frag, data_stride, frag_data, border);
    m_need_update = true;
    m_allocated_pixels += frag->m_space->width * frag->m_row->height;
    return frag;
  }
  return nullptr;
}

void BitmapFragmentMap::FreeFragmentSpace(BitmapFragment* frag) {
  if (!frag) return;
  assert(frag->m_map == this);

#ifdef TB_RUNTIME_DEBUG_INFO
  // Debug code to clear the area in debug builds so it's easier to
  // see & debug the allocation & deallocation of fragments in maps.
  uint32_t* data32 = new uint32_t[frag->m_space->width * frag->m_row->height];
  static int c = 0;
  std::memset(data32, (c++) * 32,
              sizeof(uint32_t) * frag->m_space->width * frag->m_row->height);
  CopyData(frag, frag->m_space->width, data32, false);
  m_need_update = true;
  delete[] data32;
#endif  // TB_RUNTIME_DEBUG_INFO

  m_allocated_pixels -= frag->m_space->width * frag->m_row->height;
  frag->m_row->FreeSpace(frag->m_space);
  frag->m_space = nullptr;
  frag->m_row_height = 0;

  // If the row is now empty, merge empty rows so larger fragments
  // have a chance of allocating the space.
  if (frag->m_row->IsAllAvailable()) {
    for (size_t i = 0; i < m_rows.size() - 1; ++i) {
      assert(i >= 0);
      assert(i < m_rows.size() - 1);
      auto row = m_rows[i].get();
      auto next_row = m_rows[i].get();
      if (row->IsAllAvailable() && next_row->IsAllAvailable()) {
        row->height += next_row->height;
        m_rows.erase(m_rows.begin() + i + 1);
        i--;
      }
    }
  }
}

void BitmapFragmentMap::CopyData(BitmapFragment* frag, int data_stride,
                                 uint32_t* frag_data, int border) {
  // Copy the bitmap data.
  uint32_t* dst = m_bitmap_data + frag->m_rect.x + frag->m_rect.y * m_bitmap_w;
  uint32_t* src = frag_data;
  for (int i = 0; i < frag->m_rect.h; ++i) {
    std::memcpy(dst, src, frag->m_rect.w * sizeof(uint32_t));
    dst += m_bitmap_w;
    src += data_stride;
  }
  // Copy the bitmap data to the border around the fragment.
  if (border) {
    Rect rect = frag->m_rect.Expand(border, border);
    // Copy vertical edges.
    dst = m_bitmap_data + rect.x + (rect.y + 1) * m_bitmap_w;
    src = frag_data;
    for (int i = 0; i < frag->m_rect.h; ++i) {
      dst[0] = src[0] & 0x00ffffff;
      dst[rect.w - 1] = src[frag->m_rect.w - 1] & 0x00ffffff;
      dst += m_bitmap_w;
      src += data_stride;
    }
    // Copy horizontal edges.
    dst = m_bitmap_data + rect.x + 1 + rect.y * m_bitmap_w;
    src = frag_data;
    for (int i = 0; i < frag->m_rect.w; ++i) {
      dst[i] = src[i] & 0x00ffffff;
    }
    dst = m_bitmap_data + rect.x + 1 + (rect.y + rect.h - 1) * m_bitmap_w;
    src = frag_data + (frag->m_rect.h - 1) * data_stride;
    for (int i = 0; i < frag->m_rect.w; ++i) {
      dst[i] = src[i] & 0x00ffffff;
    }
  }
}

Bitmap* BitmapFragmentMap::GetBitmap(Validate validate_type) {
  if (m_bitmap && validate_type == Validate::kFirstTime) {
    return m_bitmap;
  }
  ValidateBitmap();
  return m_bitmap;
}

bool BitmapFragmentMap::ValidateBitmap() {
  if (m_need_update) {
    if (m_bitmap) {
      m_bitmap->SetData(m_bitmap_data);
    } else {
      m_bitmap =
          Renderer::get()->CreateBitmap(m_bitmap_w, m_bitmap_h, m_bitmap_data);
    }
    m_need_update = false;
  }
  return m_bitmap ? true : false;
}

void BitmapFragmentMap::DeleteBitmap() {
  delete m_bitmap;
  m_bitmap = nullptr;
  m_need_update = true;
}

}  // namespace resources
}  // namespace tb
