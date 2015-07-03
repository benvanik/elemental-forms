/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include <cstdint>

#include "tb_bitmap_fragment.h"
#include "tb_system.h"

#ifdef TB_IMAGE_LOADER_STB

namespace tb {

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
  int width = 0, height = 0;
  unsigned char* data = nullptr;

  StbiImageLoader() = default;
  ~StbiImageLoader() { stbi_image_free(data); }

  int Width() override { return width; }
  int Height() override { return height; }
  uint32_t* Data() override { return (uint32_t*)data; }
};

ImageLoader* ImageLoader::CreateFromFile(const std::string& filename) {
  // Load directly from file.
  TBFile* file = TBFile::Open(filename, TBFile::Mode::kRead);
  if (!file) {
    return nullptr;
  }
  size_t size = file->Size();
  unsigned char* data = new unsigned char[size];
  size = file->Read(data, 1, size);

  int w, h, comp;
  if (unsigned char* img_data =
          stbi_load_from_memory(data, int(size), &w, &h, &comp, 4)) {
    if (StbiImageLoader* img = new StbiImageLoader()) {
      img->width = w;
      img->height = h;
      img->data = img_data;
      delete[] data;
      delete file;
      return img;
    } else {
      stbi_image_free(img_data);
    }
  }

  delete data;
  delete file;
  return nullptr;
}

}  // namespace tb

#endif  // TB_IMAGE_LOADER_STB
