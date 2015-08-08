/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_BITMAP_FRAGMENT_MANAGER_H_
#define EL_GRAPHICS_BITMAP_FRAGMENT_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "el/id.h"

namespace el {
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

  bool has_border() const { return m_add_border; }
  // Sets whether a 1px border should be added to new fragments so stretched
  // drawing won't get filtering artifacts at the edges (default is disabled).
  void set_has_border(bool add_border) { m_add_border = add_border; }

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

  int default_map_width() const { return m_default_map_w; }
  int default_map_height() const { return m_default_map_h; }
  // Sets the default size of new fragment maps. These must be power of two.
  void SetDefaultMapSize(int w, int h);

  // Gets the amount (in percent) of space that is currently occupied by all
  // maps in this fragment manager.
  int GetUseRatio() const;

#ifdef EL_RUNTIME_DEBUG_INFO
  // Renders the maps on screen, to analyze fragment positioning.
  void Debug();
#endif  // EL_RUNTIME_DEBUG_INFO

 private:
  std::vector<std::unique_ptr<BitmapFragmentMap>> m_fragment_maps;
  std::unordered_map<uint32_t, std::unique_ptr<BitmapFragment>> m_fragments;
  int m_num_maps_limit = 0;
  bool m_add_border = false;
  int m_default_map_w = 512;
  int m_default_map_h = 512;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_BITMAP_FRAGMENT_MANAGER_H_
