/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cstdint>
#include <vector>

#include "tb/graphics/image_loader.h"
#include "tb/util/file.h"

namespace tb {
namespace graphics {

// Configure stb image and remove some features we don't use to reduce binary
// size.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
//#define STBI_SIMD
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_HDR

// Include stb image - Tiny portable and reasonable fast image loader from
// http://nothings.org/
// Should not be used for content not distributed with your app (May not be
// secure and doesn't
// support all formats fully)
#include "thirdparty/stb_image.h"

class StbiImageLoader : public ImageLoader {
 public:
  StbiImageLoader(int width, int height, uint8_t* data)
      : width_(width), height_(height), data_(data) {}
  ~StbiImageLoader() { stbi_image_free(data_); }

  int width() override { return width_; }
  int height() override { return height_; }
  uint32_t* data() override { return (uint32_t*)data_; }

 private:
  int width_;
  int height_;
  uint8_t* data_;
};

std::unique_ptr<ImageLoader> ImageLoader::CreateFromFile(
    const std::string& filename) {
  // Load directly from file.
  auto file = util::File::Open(filename, util::File::Mode::kRead);
  if (!file) {
    return nullptr;
  }
  size_t size = file->Size();
  std::vector<uint8_t> buffer(size);
  size = file->Read(buffer.data(), 1, size);

  int w, h, comp;
  auto img_data =
      stbi_load_from_memory(buffer.data(), int(size), &w, &h, &comp, 4);
  if (!img_data) {
    return nullptr;
  }

  auto img = std::make_unique<StbiImageLoader>(w, h, img_data);
  return std::unique_ptr<ImageLoader>(img.release());
}

}  // namespace graphics
}  // namespace tb
