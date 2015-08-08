/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdint>
#include <vector>

#include "el/graphics/image_loader.h"
#include "el/io/file_manager.h"

namespace el {
namespace graphics {

// Configure stb image and remove some features we don't use to reduce binary
// size.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_HDR

// Include stb image - Tiny portable and reasonable fast image loader from
// http://nothings.org/
// Should not be used for content not distributed with your app (May not be
// secure and doesn't
// support all formats fully)
#include "third_party/stb/stb_image.h"

class StbiImageLoader : public ImageLoader {
 public:
  StbiImageLoader(int width, int height, uint8_t* data)
      : width_(width), height_(height), data_(data) {}
  ~StbiImageLoader() { stbi_image_free(data_); }

  int width() override { return width_; }
  int height() override { return height_; }
  uint32_t* data() override { return reinterpret_cast<uint32_t*>(data_); }

 private:
  int width_;
  int height_;
  uint8_t* data_;
};

std::unique_ptr<ImageLoader> ImageLoader::CreateFromFile(
    const std::string& filename) {
  // Load directly from file.
  auto buffer = io::FileManager::ReadContents(filename);
  if (!buffer) {
    return nullptr;
  }

  int w, h, comp;
  auto img_data = stbi_load_from_memory(
      buffer->data(), static_cast<int>(buffer->size()), &w, &h, &comp, 4);
  if (!img_data) {
    return nullptr;
  }

  auto img = std::make_unique<StbiImageLoader>(w, h, img_data);
  return std::unique_ptr<ImageLoader>(img.release());
}

}  // namespace graphics
}  // namespace el
