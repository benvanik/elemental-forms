/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_GRAPHICS_BITMAP_FRAGMENT_MANAGER_H_
#define TB_GRAPHICS_BITMAP_FRAGMENT_MANAGER_H_

#include <memory>
#include <vector>

#include "tb/id.h"
#include "tb/util/hash_table.h"

namespace tb {
namespace graphics {

class BitmapFragment;
class BitmapFragmentMap;

// Manages loading bitmaps of arbitrary size, pack as many of them into as few
// Bitmap as possible.
// It also makes sure that only one instance of each file is loaded, so f.ex
// loading "foo.png" many times still load and allocate one BitmapFragment.
class BitmapFragmentManager {
 public:
  BitmapFragmentManager();
  ~BitmapFragmentManager();

  // Sets whether a 1px border should be added to new fragments so stretched
  // drawing won't get filtering artifacts at the edges (default is disabled).
  void SetAddBorder(bool add_border) { m_add_border = add_border; }
  bool GetAddBorder() const { return m_add_border; }

  // Gets the fragment with the given image filename.
  // If it's not already loaded, it will be loaded into a new fragment with the
  // filename as id. returns nullptr on fail.
  BitmapFragment* GetFragmentFromFile(const std::string& filename,
                                      bool dedicated_map);

  // Gets the fragment with the given id, or nullptr if it doesn't exist.
  BitmapFragment* GetFragment(const TBID& id) const;

  // Creates a new fragment from the given data.
  // @param id The id that should be used to identify the fragment.
  // @param dedicated_map if true, it will get a dedicated map.
  // @param data_w the width of the data.
  // @param data_h the height of the data.
  // @param data_stride the number of pixels in a row of the input data.
  // @param data pointer to the data in BGRA32 format.
  BitmapFragment* CreateNewFragment(const TBID& id, bool dedicated_map,
                                    int data_w, int data_h, int data_stride,
                                    uint32_t* data);

  // Deletes the given fragment and free the space it used in its map, so that
  // other fragments can take its place.
  void FreeFragment(BitmapFragment* frag);

  // Clears all loaded bitmaps and all created bitmap fragments and maps.
  // After this call, do not keep any pointers to any BitmapFragment created by
  // this fragment manager.
  void Clear();

  // Validates bitmaps on fragment maps that has changed.
  bool ValidateBitmaps();

  // Delete all bitmaps in all fragment maps in this manager.
  // The bitmaps will be recreated automatically when needed, or when calling
  // ValidateBitmaps. You do not need to call this, except when the context is
  // lost and all bitmaps must be forgotten.
  void DeleteBitmaps();

  // Gets number of fragment maps that is currently used.
  size_t map_count() const { return m_fragment_maps.size(); }

  // Sets the number of maps (Bitmaps) this manager should be allowed to
  // create.
  // If a new fragment can't fit into any existing bitmap and the limit is
  // reached, the fragment creation will fail. Set to 0 for unlimited (default).
  void SetNumMapsLimit(int num_maps_limit);

  // Sets the default size of new fragment maps. These must be power of two.
  void SetDefaultMapSize(int w, int h);

  // Gets the amount (in percent) of space that is currently occupied by all
  // maps in this fragment manager.
  int GetUseRatio() const;

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the maps on screen, to analyze fragment positioning.
  void Debug();
#endif  // TB_RUNTIME_DEBUG_INFO

 private:
  std::vector<std::unique_ptr<BitmapFragmentMap>> m_fragment_maps;
  util::HashTableOf<BitmapFragment> m_fragments;
  int m_num_maps_limit = 0;
  bool m_add_border = false;
  int m_default_map_w = 512;
  int m_default_map_h = 512;
};

}  // namespace graphics
}  // namespace tb

#endif  // TB_GRAPHICS_BITMAP_FRAGMENT_MANAGER_H_
