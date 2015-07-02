/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_BITMAP_FRAGMENT_H
#define TB_BITMAP_FRAGMENT_H

#include "tb_core.h"
#include "tb_geometry.h"
#include "tb_hashtable.h"
#include "tb_id.h"
#include "tb_linklist.h"
#include "tb_list.h"

namespace tb {

class Bitmap;
class BitmapFragment;

// Returns the nearest power of two from val.
// F.ex 110 -> 128, 256->256, 257->512 etc.
int GetNearestPowerOfTwo(int val);

// A class used to load skin images. It can be implemented in any way the system
// wants, but the system has to provide a image loader capable of handling all
// image formats used in the skin.
class ImageLoader {
 public:
  // The system must implement this function and create an implementation of the
  // ImageLoader interface.
  static ImageLoader* CreateFromFile(const std::string& filename);

  virtual ~ImageLoader() = default;

  virtual int Width() = 0;
  virtual int Height() = 0;
  // This data should always be in 32bit RGBA format.
  virtual uint32_t* Data() = 0;
};

// Allocates of space out of a given available space.
class SpaceAllocator {
 public:
  // A chunk of space.
  class Space : public TBLinkOf<Space> {
   public:
    int x;
    int width;
  };

  SpaceAllocator(int available_space) : m_available_space(available_space) {}

  // Returns true if no allocations are currently live using this allocator.
  bool IsAllAvailable() const { return !m_used_space_list.HasLinks(); }
  // Returns true if the given width is currently available.
  bool HasSpace(int needed_w) const;
  // Allocates the given space and return the Space, or nullptr on error.
  Space* AllocSpace(int needed_w);
  // Frees the given space so it is available for new allocations.
  void FreeSpace(Space* space);

 private:
  Space* GetSmallestAvailableSpace(int needed_w);

  int m_available_space;
  TBLinkListAutoDeleteOf<Space> m_free_space_list;
  TBLinkListAutoDeleteOf<Space> m_used_space_list;
};

// Allocates space for BitmapFragment in a row (used in BitmapFragmentMap).
class BitmapFragmentSpaceAllocator : public SpaceAllocator {
 public:
  BitmapFragmentSpaceAllocator(int y, int width, int height)
      : SpaceAllocator(width), y(y), height(height) {}
  int y;
  int height;
};

// Specify when the bitmap should be validated when calling
// BitmapFragmentMap::GetBitmap.
enum class Validate {
  // Always validate the bitmap (the bitmap is updated if needed).
  kAlways,
  // Only validate if the bitmap does not yet exist (make sure there is a valid
  // bitmap pointer, but the data is not necessarily updated).
  kFirstTime,
};

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

  TBListAutoDeleteOf<BitmapFragmentSpaceAllocator> m_rows;
  int m_bitmap_w = 0;
  int m_bitmap_h = 0;
  uint32_t* m_bitmap_data = nullptr;
  Bitmap* m_bitmap = nullptr;
  bool m_need_update = false;
  int m_allocated_pixels = 0;
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
  Bitmap* GetBitmap(Validate validate_type = Validate::kAlways) {
    return m_map->GetBitmap(validate_type);
  }

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
  int GetNumMaps() const { return m_fragment_maps.GetNumItems(); }

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
  TBListOf<BitmapFragmentMap> m_fragment_maps;
  TBHashTableOf<BitmapFragment> m_fragments;
  int m_num_maps_limit = 0;
  bool m_add_border = false;
  int m_default_map_w = 512;
  int m_default_map_h = 512;
};

}  // namespace tb

#endif  // TB_BITMAP_FRAGMENT_H
