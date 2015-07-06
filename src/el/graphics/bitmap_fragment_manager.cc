/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>

#include "el/graphics/bitmap_fragment.h"
#include "el/graphics/bitmap_fragment_manager.h"
#include "el/graphics/bitmap_fragment_map.h"
#include "el/graphics/image_loader.h"
#include "el/graphics/renderer.h"
#include "el/util/math.h"
#include "el/util/space_allocator.h"

namespace el {
namespace graphics {

BitmapFragmentManager::BitmapFragmentManager() = default;

BitmapFragmentManager::~BitmapFragmentManager() = default;

BitmapFragment* BitmapFragmentManager::GetFragmentFromFile(
    const std::string& filename, bool dedicated_map) {
  TBID id(filename);

  // If we already have a fragment for this filename, return that.
  auto& it = m_fragments.find(id);
  if (it != m_fragments.end()) {
    return it->second.get();
  }

  // Load the file.
  auto img = ImageLoader::CreateFromFile(filename);
  if (!img) {
    return nullptr;
  }

  return CreateNewFragment(id, dedicated_map, img->width(), img->height(),
                           img->width(), img->data());
}

BitmapFragment* BitmapFragmentManager::CreateNewFragment(const TBID& id,
                                                         bool dedicated_map,
                                                         int data_w, int data_h,
                                                         int data_stride,
                                                         uint32_t* data) {
  assert(!GetFragment(id));

  std::unique_ptr<BitmapFragment> fragment;

  // Create a fragment in any of the fragment maps. Doing it in the reverse
  // order would be faster since it's most likely to succeed, but we want to
  // maximize the amount of fragments per map, so do it in the creation order.
  if (!dedicated_map) {
    for (auto& fragment_map : m_fragment_maps) {
      if ((fragment = fragment_map->CreateNewFragment(
               data_w, data_h, data_stride, data, m_add_border))) {
        break;
      }
    }
  }
  // If we couldn't create the fragment in any map, create a new map where we
  // know it will fit.
  bool allow_another_map =
      m_num_maps_limit == 0 || m_fragment_maps.size() < m_num_maps_limit;
  if (!fragment && allow_another_map) {
    int po2w = util::GetNearestPowerOfTwo(std::max(data_w, m_default_map_w));
    int po2h = util::GetNearestPowerOfTwo(std::max(data_h, m_default_map_h));
    if (dedicated_map) {
      po2w = util::GetNearestPowerOfTwo(data_w);
      po2h = util::GetNearestPowerOfTwo(data_h);
    }
    auto fragment_map = std::make_unique<BitmapFragmentMap>();
    if (fragment_map->Init(po2w, po2h)) {
      fragment = fragment_map->CreateNewFragment(data_w, data_h, data_stride,
                                                 data, m_add_border);
      m_fragment_maps.push_back(std::move(fragment_map));
    }
  }
  // Finally, add the new fragment to the hash.
  if (fragment) {
    fragment->m_id = id;
    auto fragment_ptr = fragment.get();
    m_fragments.emplace(id, std::move(fragment));
    return fragment_ptr;
  }
  return nullptr;
}

void BitmapFragmentManager::FreeFragment(BitmapFragment* frag) {
  if (frag) {
    Renderer::get()->FlushBitmapFragment(frag);

    BitmapFragmentMap* map = frag->m_map;
    frag->m_map->FreeFragmentSpace(frag);
    m_fragments.erase(frag->m_id);

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
  auto& it = m_fragments.find(id);
  return it != m_fragments.end() ? it->second.get() : nullptr;
}

void BitmapFragmentManager::Clear() {
  m_fragment_maps.clear();
  m_fragments.clear();
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

#ifdef EL_RUNTIME_DEBUG_INFO
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
#endif  // EL_RUNTIME_DEBUG_INFO

}  // namespace graphics
}  // namespace el
