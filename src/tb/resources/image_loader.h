/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_IMAGE_LOADER_H_
#define TB_RESOURCES_IMAGE_LOADER_H_

#include <memory>
#include <string>

namespace tb {
namespace resources {

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

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_IMAGE_LOADER_H_
