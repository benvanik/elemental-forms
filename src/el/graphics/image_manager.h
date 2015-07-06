/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_IMAGE_MANAGER_H_
#define EL_GRAPHICS_IMAGE_MANAGER_H_

#include <memory>
#include <unordered_map>

#include "el/graphics/bitmap_fragment_manager.h"
#include "el/graphics/renderer.h"

namespace el {
namespace graphics {

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
  int width() const;

  // Returns the height of this image, or 0 if empty.
  int height() const;

  // Returns the bitmap fragment for this image, or nullptr if empty.
  BitmapFragment* bitmap() const;

  const Image& operator=(const Image& image) {
    set_image_rep(image.m_image_rep);
    return *this;
  }
  bool operator==(const Image& image) const {
    return m_image_rep == image.m_image_rep;
  }
  bool operator!=(const Image& image) const {
    return m_image_rep != image.m_image_rep;
  }

 private:
  void set_image_rep(ImageRep* image_rep);

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
  static ImageManager* get() { return image_manager_singleton_.get(); }
  static void set(std::unique_ptr<ImageManager> value) {
    image_manager_singleton_ = std::move(value);
  }

  ImageManager();
  ~ImageManager();

  // Returns an image object for the given filename.
  // If it fails, the returned Image object will be empty.
  Image GetImage(const char* filename);

#ifdef EL_RUNTIME_DEBUG_INFO
  // Renders the skin bitmaps on screen, to analyze fragment positioning.
  void Debug() { m_frag_manager.Debug(); }
#endif  // EL_RUNTIME_DEBUG_INFO

  void OnContextLost() override;
  void OnContextRestored() override;

 private:
  friend class ImageRep;
  void RemoveImageRep(ImageRep* image_rep);

  static std::unique_ptr<ImageManager> image_manager_singleton_;

  BitmapFragmentManager m_frag_manager;
  std::unordered_map<uint32_t, ImageRep*> m_image_rep_hash;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_IMAGE_MANAGER_H_
