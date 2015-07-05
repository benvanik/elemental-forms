/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_GRAPHICS_IMAGE_LOADER_H_
#define TB_GRAPHICS_IMAGE_LOADER_H_

#include <memory>
#include <string>

namespace tb {
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
}  // namespace tb

#endif  // TB_GRAPHICS_IMAGE_LOADER_H_
