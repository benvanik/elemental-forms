/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_IMAGE_MANAGER_H
#define TB_IMAGE_MANAGER_H

#include "tb_bitmap_fragment.h"
#include "tb_core.h"
#include "tb_renderer.h"

#include "tb/util/hash_table.h"

namespace tb {

class ImageManager;

// ImageRep is the internal contents of a Image.
// Owned by reference counting from Image.
class ImageRep {
  friend class ImageManager;
  friend class Image;

  ImageRep(ImageManager* image_manager, BitmapFragment* fragment,
           uint32_t hash_key);

  void IncRef();
  void DecRef();

  int ref_count = 0;
  uint32_t hash_key;
  ImageManager* image_manager;
  BitmapFragment* fragment;
};

// A reference counting object representing a image loaded by ImageManager.
// As long as there are Image objects for a certain image, it will be kept
// loaded in memory.
// It may be empty if the image has not yet been set, or if the ImageManager is
// destroyed when the image is still alive.
class Image {
 public:
  Image() = default;
  Image(ImageRep* rep);
  Image(const Image& image);
  ~Image();

  // Returns true if this image is empty.
  bool empty() const;

  // Returns the width of this image, or 0 if empty.
  int Width() const;

  // Returns the height of this image, or 0 if empty.
  int Height() const;

  // Returns the bitmap fragment for this image, or nullptr if empty.
  BitmapFragment* GetBitmap() const;

  const Image& operator=(const Image& image) {
    SetImageRep(image.m_image_rep);
    return *this;
  }
  bool operator==(const Image& image) const {
    return m_image_rep == image.m_image_rep;
  }
  bool operator!=(const Image& image) const {
    return m_image_rep != image.m_image_rep;
  }

 private:
  void SetImageRep(ImageRep* image_rep);

  ImageRep* m_image_rep = nullptr;
};

// Loads images returned as Image objects.
// It internally use a BitmapFragmentManager that create fragment maps for
// loaded images, and keeping track of which images are loaded so they are not
// loaded several times.
// Images are forgotten when there are no longer any Image objects for a given
// file.
class ImageManager : private RendererListener {
 public:
  ImageManager();
  ~ImageManager();

  // Returns an image object for the given filename.
  // If it fails, the returned Image object will be empty.
  Image GetImage(const char* filename);

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the skin bitmaps on screen, to analyze fragment positioning.
  void Debug() { m_frag_manager.Debug(); }
#endif  // TB_RUNTIME_DEBUG_INFO

  void OnContextLost() override;
  void OnContextRestored() override;

 private:
  friend class ImageRep;
  void RemoveImageRep(ImageRep* image_rep);

  BitmapFragmentManager m_frag_manager;
  util::HashTableOf<ImageRep> m_image_rep_hash;
};

// The global ImageManager.
extern ImageManager* g_image_manager;

}  // namespace tb

#endif  // TB_IMAGE_MANAGER_H
