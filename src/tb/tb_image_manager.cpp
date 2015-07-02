/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_image_manager.h"

#include "tb_hash.h"
#include "tb_skin.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"

namespace tb {

ImageRep::ImageRep(ImageManager* image_manager, BitmapFragment* fragment,
                   uint32_t hash_key)
    : hash_key(hash_key), image_manager(image_manager), fragment(fragment) {}

void ImageRep::IncRef() { ref_count++; }

void ImageRep::DecRef() {
  ref_count--;
  if (ref_count == 0) {
    if (image_manager) {
      image_manager->RemoveImageRep(this);
    }
    delete this;
  }
}

Image::Image(ImageRep* rep) : m_image_rep(rep) {
  if (m_image_rep) m_image_rep->IncRef();
}

Image::Image(const Image& image) : m_image_rep(image.m_image_rep) {
  if (m_image_rep) m_image_rep->IncRef();
}

Image::~Image() {
  if (m_image_rep) m_image_rep->DecRef();
}

bool Image::IsEmpty() const { return m_image_rep && m_image_rep->fragment; }

int Image::Width() const {
  if (m_image_rep && m_image_rep->fragment) {
    return m_image_rep->fragment->Width();
  }
  return 0;
}

int Image::Height() const {
  if (m_image_rep && m_image_rep->fragment) {
    return m_image_rep->fragment->Height();
  }
  return 0;
}

BitmapFragment* Image::GetBitmap() const {
  return m_image_rep ? m_image_rep->fragment : nullptr;
}

void Image::SetImageRep(ImageRep* image_rep) {
  if (m_image_rep == image_rep) {
    return;
  }
  if (m_image_rep) {
    m_image_rep->DecRef();
  }
  m_image_rep = image_rep;
  if (m_image_rep) {
    m_image_rep->IncRef();
  }
}

ImageManager* g_image_manager = nullptr;

ImageManager::ImageManager() { g_renderer->AddListener(this); }

ImageManager::~ImageManager() {
  g_renderer->RemoveListener(this);

  // If there is ImageRep objects live, we must unset the fragment pointer
  // since the m_frag_manager is going to be destroyed very soon.
  TBHashTableIteratorOf<ImageRep> it(&m_image_rep_hash);
  while (ImageRep* image_rep = it.GetNextContent()) {
    image_rep->fragment = nullptr;
    image_rep->image_manager = nullptr;
  }
}

Image ImageManager::GetImage(const char* filename) {
  uint32_t hash_key = TBGetHash(filename);
  ImageRep* image_rep = m_image_rep_hash.Get(hash_key);
  if (!image_rep) {
    // Load a fragment. Load a destination DPI bitmap if available.
    BitmapFragment* fragment = nullptr;
    if (g_tb_skin->GetDimensionConverter()->NeedConversion()) {
      TBTempBuffer filename_dst_DPI;
      g_tb_skin->GetDimensionConverter()->GetDstDPIFilename(filename,
                                                            &filename_dst_DPI);
      fragment =
          m_frag_manager.GetFragmentFromFile(filename_dst_DPI.GetData(), false);
    }
    if (!fragment) {
      fragment = m_frag_manager.GetFragmentFromFile(filename, false);
    }
    if (fragment) {
      image_rep = new ImageRep(this, fragment, hash_key);
      m_image_rep_hash.Add(hash_key, image_rep);
    }
    TBDebugOut(image_rep ? "ImageManager - Loaded new image.\n"
                         : "ImageManager - Loading image failed.\n");
  }
  return Image(image_rep);
}

void ImageManager::RemoveImageRep(ImageRep* image_rep) {
  assert(image_rep->ref_count == 0);
  if (image_rep->fragment) {
    m_frag_manager.FreeFragment(image_rep->fragment);
    image_rep->fragment = nullptr;
  }
  m_image_rep_hash.Remove(image_rep->hash_key);
  image_rep->image_manager = nullptr;
  TBDebugOut("ImageManager - Removed image.\n");
}

void ImageManager::OnContextLost() { m_frag_manager.DeleteBitmaps(); }

void ImageManager::OnContextRestored() {
  // No need to do anything. The bitmaps will be created when drawing.
}

}  // namespace tb
