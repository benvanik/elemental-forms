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

class STBI_Loader : public TBImageLoader {
 public:
  int width, height;
  unsigned char* data;

  STBI_Loader() : width(0), height(0), data(nullptr) {}
  ~STBI_Loader() { stbi_image_free(data); }

  virtual int Width() { return width; }
  virtual int Height() { return height; }
  virtual uint32_t* Data() { return (uint32_t*)data; }
};

TBImageLoader* TBImageLoader::CreateFromFile(const char* filename) {
  // Load directly from file
  /*int w, h, comp;
  if (unsigned char *data = stbi_load(filename, &w, &h, &comp, 4))
  {
          if (STBI_Loader *img = new STBI_Loader())
          {
                  img->width = w;
                  img->height = h;
                  img->data = data;
                  return img;
          }
          else
                  stbi_image_free(data);
  }
  return nullptr;*/
  if (TBFile* file = TBFile::Open(filename, TBFile::MODE_READ)) {
    size_t size = file->Size();
    if (unsigned char* data = new unsigned char[size]) {
      size = file->Read(data, 1, size);

      int w, h, comp;
      if (unsigned char* img_data =
              stbi_load_from_memory(data, int(size), &w, &h, &comp, 4)) {
        if (STBI_Loader* img = new STBI_Loader()) {
          img->width = w;
          img->height = h;
          img->data = img_data;
          delete[] data;
          delete file;
          return img;
        } else
          stbi_image_free(img_data);
      }

      delete data;
    }
    delete file;
  }
  return nullptr;
}

}  // namespace tb

#endif  // TB_IMAGE_LOADER_STB
