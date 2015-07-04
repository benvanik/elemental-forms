/**
******************************************************************************
* xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
******************************************************************************
* Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
* See turbo_badger.h and LICENSE in the root for more information.           *
******************************************************************************
*/

#include <algorithm>

#include "tb/graphics/bitmap_fragment.h"
#include "tb/graphics/bitmap_fragment_manager.h"
#include "tb/graphics/bitmap_fragment_map.h"
#include "tb/graphics/image_loader.h"
#include "tb/graphics/renderer.h"
#include "tb/util/math.h"
#include "tb/util/space_allocator.h"

namespace tb {
namespace graphics {

BitmapFragmentManager::BitmapFragmentManager() = default;

BitmapFragmentManager::~BitmapFragmentManager() { Clear(); }

BitmapFragment* BitmapFragmentManager::GetFragmentFromFile(
    const std::string& filename, bool dedicated_map) {
  TBID id(filename);

  // If we already have a fragment for this filename, return that
  BitmapFragment* frag = m_fragments.Get(id);
  if (frag) {
    return frag;
  }

  // Load the file
  ImageLoader* img = ImageLoader::CreateFromFile(filename);
  if (!img) {
    return nullptr;
  }

  frag = CreateNewFragment(id, dedicated_map, img->Width(), img->Height(),
                           img->Width(), img->Data());
  delete img;
  return frag;
}

BitmapFragment* BitmapFragmentManager::CreateNewFragment(const TBID& id,
                                                         bool dedicated_map,
                                                         int data_w, int data_h,
                                                         int data_stride,
                                                         uint32_t* data) {
  assert(!GetFragment(id));

  BitmapFragment* frag = nullptr;

  // Create a fragment in any of the fragment maps. Doing it in the reverse
  // order would be faster since it's most likely to succeed, but we want to
  // maximize the amount of fragments per map, so do it in the creation order.
  if (!dedicated_map) {
    for (auto& fragment_map : m_fragment_maps) {
      if ((frag = fragment_map->CreateNewFragment(data_w, data_h, data_stride,
                                                  data, m_add_border))) {
        break;
      }
    }
  }
  // If we couldn't create the fragment in any map, create a new map where we
  // know it will fit.
  bool allow_another_map =
      m_num_maps_limit == 0 || m_fragment_maps.size() < m_num_maps_limit;
  if (!frag && allow_another_map) {
    int po2w = util::GetNearestPowerOfTwo(std::max(data_w, m_default_map_w));
    int po2h = util::GetNearestPowerOfTwo(std::max(data_h, m_default_map_h));
    if (dedicated_map) {
      po2w = util::GetNearestPowerOfTwo(data_w);
      po2h = util::GetNearestPowerOfTwo(data_h);
    }
    auto fm = std::make_unique<BitmapFragmentMap>();
    if (fm->Init(po2w, po2h)) {
      frag = fm->CreateNewFragment(data_w, data_h, data_stride, data,
                                   m_add_border);
      m_fragment_maps.push_back(std::move(fm));
    }
  }
  // Finally, add the new fragment to the hash.
  if (frag) {
    m_fragments.Add(id, frag);
    frag->m_id = id;
    return frag;
  }
  return nullptr;
}

void BitmapFragmentManager::FreeFragment(BitmapFragment* frag) {
  if (frag) {
    Renderer::get()->FlushBitmapFragment(frag);

    BitmapFragmentMap* map = frag->m_map;
    frag->m_map->FreeFragmentSpace(frag);
    m_fragments.Delete(frag->m_id);

    // If the map is now empty, delete it.
    if (map->m_allocated_pixels == 0) {
      for (auto it = m_fragment_maps.begin(); it != m_fragment_maps.end();
           ++it) {
        if (it->get() == map) {
          m_fragment_maps.erase(it);
          break;
        }
      }
    }
  }
}

BitmapFragment* BitmapFragmentManager::GetFragment(const TBID& id) const {
  return m_fragments.Get(id);
}

void BitmapFragmentManager::Clear() {
  m_fragment_maps.clear();
  m_fragments.DeleteAll();
}

bool BitmapFragmentManager::ValidateBitmaps() {
  bool success = true;
  for (auto& it : m_fragment_maps) {
    if (!it->ValidateBitmap()) {
      success = false;
    }
  }
  return success;
}

void BitmapFragmentManager::DeleteBitmaps() {
  for (auto& it : m_fragment_maps) {
    it->DeleteBitmap();
  }
}

void BitmapFragmentManager::SetNumMapsLimit(int num_maps_limit) {
  m_num_maps_limit = num_maps_limit;
}

void BitmapFragmentManager::SetDefaultMapSize(int w, int h) {
  assert(util::GetNearestPowerOfTwo(w) == w);
  assert(util::GetNearestPowerOfTwo(h) == h);
  m_default_map_w = w;
  m_default_map_h = h;
}

int BitmapFragmentManager::GetUseRatio() const {
  int used = 0;
  int total = 0;
  for (auto& fragment_map : m_fragment_maps) {
    used += fragment_map->m_allocated_pixels;
    total += fragment_map->m_bitmap_w * fragment_map->m_bitmap_h;
  }
  return total ? (used * 100) / total : 0;
}

#ifdef TB_RUNTIME_DEBUG_INFO
void BitmapFragmentManager::Debug() {
  int x = 0;
  for (auto& fragment_map : m_fragment_maps) {
    if (Bitmap* bitmap = fragment_map->GetBitmap()) {
      Renderer::get()->DrawBitmap(
          Rect(x, 0, fragment_map->m_bitmap_w, fragment_map->m_bitmap_h),
          Rect(0, 0, fragment_map->m_bitmap_w, fragment_map->m_bitmap_h),
          bitmap);
    }
    x += fragment_map->m_bitmap_w + 5;
  }
}
#endif  // TB_RUNTIME_DEBUG_INFO

}  // namespace graphics
}  // namespace tb
