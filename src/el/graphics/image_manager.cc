/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/graphics/image_manager.h"
#include "el/skin.h"
#include "el/util/debug.h"
#include "el/util/hash.h"
#include "el/util/string_builder.h"

namespace el {
namespace graphics {

std::unique_ptr<ImageManager> ImageManager::image_manager_singleton_;

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

bool Image::empty() const { return m_image_rep && m_image_rep->fragment; }

int Image::width() const {
  if (m_image_rep && m_image_rep->fragment) {
    return m_image_rep->fragment->width();
  }
  return 0;
}

int Image::height() const {
  if (m_image_rep && m_image_rep->fragment) {
    return m_image_rep->fragment->height();
  }
  return 0;
}

BitmapFragment* Image::bitmap() const {
  return m_image_rep ? m_image_rep->fragment : nullptr;
}

void Image::set_image_rep(ImageRep* image_rep) {
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

ImageManager::ImageManager() { Renderer::get()->AddListener(this); }

ImageManager::~ImageManager() {
  Renderer::get()->RemoveListener(this);

  // If there is ImageRep objects live, we must unset the fragment pointer
  // since the m_frag_manager is going to be destroyed very soon.
  for (auto& it : m_image_rep_hash) {
    it.second->fragment = nullptr;
    it.second->image_manager = nullptr;
  }
}

Image ImageManager::GetImage(const char* filename) {
  uint32_t hash_key = util::hash(filename);
  auto it = m_image_rep_hash.find(hash_key);
  if (it != m_image_rep_hash.end()) {
    return it->second;
  }

  // Load a fragment. Load a destination DPI bitmap if available.
  BitmapFragment* fragment = nullptr;
  auto dimension_converter = Skin::get()->dimension_converter();
  if (dimension_converter->NeedConversion()) {
    util::StringBuilder filename_dst_DPI;
    dimension_converter->GetDstDPIFilename(filename, &filename_dst_DPI);
    fragment =
        m_frag_manager.GetFragmentFromFile(filename_dst_DPI.c_str(), false);
  }
  if (!fragment) {
    fragment = m_frag_manager.GetFragmentFromFile(filename, false);
  }
  if (!fragment) {
    TBDebugOut("ImageManager - Loading image failed: %s\n", filename);
    return Image(nullptr);
  }
  ImageRep* image_rep = new ImageRep(this, fragment, hash_key);
  m_image_rep_hash.emplace(hash_key, image_rep);
  return Image(image_rep);
}

void ImageManager::RemoveImageRep(ImageRep* image_rep) {
  assert(image_rep->ref_count == 0);
  if (image_rep->fragment) {
    m_frag_manager.FreeFragment(image_rep->fragment);
    image_rep->fragment = nullptr;
  }
  m_image_rep_hash.erase(image_rep->hash_key);
  image_rep->image_manager = nullptr;
  TBDebugOut("ImageManager - Removed image.\n");
}

void ImageManager::OnContextLost() { m_frag_manager.DeleteBitmaps(); }

void ImageManager::OnContextRestored() {
  // No need to do anything. The bitmaps will be created when drawing.
}

}  // namespace graphics
}  // namespace el
