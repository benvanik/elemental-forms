/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_IMAGE_LOADER_H_
#define EL_GRAPHICS_IMAGE_LOADER_H_

#include <memory>
#include <string>

namespace el {
namespace graphics {

// A class used to load skin images. It can be implemented in any way the system
// wants, but the system has to provide a image loader capable of handling all
// image formats used in the skin.
class ImageLoader {
 public:
  // The system must implement this function and create an implementation of the
  // ImageLoader interface.
  static std::unique_ptr<ImageLoader> CreateFromFile(
      const std::string& filename);

  virtual ~ImageLoader() = default;

  virtual int width() = 0;
  virtual int height() = 0;
  // This data should always be in 32bit RGBA format.
  virtual uint32_t* data() = 0;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_IMAGE_LOADER_H_
