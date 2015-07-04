/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <memory>

#include "tb/resources/font_manager.h"
#include "tb/resources/font_renderer.h"

namespace tb {
namespace resources {

using graphics::Renderer;

std::unique_ptr<FontManager> FontManager::font_manager_singleton_;

FontGlyphCache::FontGlyphCache() {
  // Only use one map for the font face. The glyph cache will start forgetting
  // glyphs that haven't been used for a while if the map gets full.
  m_frag_manager.SetNumMapsLimit(1);
  m_frag_manager.SetDefaultMapSize(TB_GLYPH_CACHE_WIDTH, TB_GLYPH_CACHE_HEIGHT);

  Renderer::get()->AddListener(this);
}

FontGlyphCache::~FontGlyphCache() { Renderer::get()->RemoveListener(this); }

FontGlyph* FontGlyphCache::GetGlyph(const TBID& hash_id, UCS4 cp) {
  if (FontGlyph* glyph = m_glyphs.Get(hash_id)) {
    // Move the glyph to the end of m_all_rendered_glyphs so we maintain LRU
    // (oldest first)
    if (m_all_rendered_glyphs.ContainsLink(glyph)) {
      m_all_rendered_glyphs.Remove(glyph);
      m_all_rendered_glyphs.AddLast(glyph);
    }
    return glyph;
  }
  return nullptr;
}

FontGlyph* FontGlyphCache::CreateAndCacheGlyph(const TBID& hash_id, UCS4 cp) {
  assert(!GetGlyph(hash_id, cp));
  FontGlyph* glyph = new FontGlyph(hash_id, cp);
  m_glyphs.Add(glyph->hash_id, glyph);
  return glyph;
}

graphics::BitmapFragment* FontGlyphCache::CreateFragment(FontGlyph* glyph,
                                                         int w, int h,
                                                         int stride,
                                                         uint32_t* data) {
  assert(GetGlyph(glyph->hash_id, glyph->cp));
  // Don't bother if the requested glyph is too large.
  if (w > TB_GLYPH_CACHE_WIDTH || h > TB_GLYPH_CACHE_HEIGHT) {
    return nullptr;
  }

  bool try_drop_largest = true;
  bool dropped_large_enough_glyph = false;
  do {
    // Attempt creating a fragment for the rendered glyph data.
    if (auto frag = m_frag_manager.CreateNewFragment(glyph->hash_id, false, w,
                                                     h, stride, data)) {
      glyph->frag = frag;
      m_all_rendered_glyphs.AddLast(glyph);
      return frag;
    }
    // Drop the oldest glyph that's large enough to free up the space we need.
    if (try_drop_largest) {
      const int check_limit = 20;
      int check_count = 0;
      for (FontGlyph* oldest = m_all_rendered_glyphs.GetFirst();
           oldest && check_count < check_limit; oldest = oldest->GetNext()) {
        if (oldest->frag->Width() >= w &&
            oldest->frag->GetAllocatedHeight() >= h) {
          DropGlyphFragment(oldest);
          dropped_large_enough_glyph = true;
          break;
        }
        check_count++;
      }
      try_drop_largest = false;
    }
    // We had no large enough glyph so just drop the oldest one. We will likely
    // spin around the loop, fail and drop again a few times before we succeed.
    if (!dropped_large_enough_glyph) {
      if (FontGlyph* oldest = m_all_rendered_glyphs.GetFirst()) {
        DropGlyphFragment(oldest);
      } else {
        break;
      }
    }
  } while (true);
  return nullptr;
}

void FontGlyphCache::DropGlyphFragment(FontGlyph* glyph) {
  assert(glyph->frag);
  m_frag_manager.FreeFragment(glyph->frag);
  glyph->frag = nullptr;
  m_all_rendered_glyphs.Remove(glyph);
}

#ifdef TB_RUNTIME_DEBUG_INFO
void FontGlyphCache::Debug() { m_frag_manager.Debug(); }
#endif  // TB_RUNTIME_DEBUG_INFO

void FontGlyphCache::OnContextLost() { m_frag_manager.DeleteBitmaps(); }

void FontGlyphCache::OnContextRestored() {
  // No need to do anything. The bitmaps will be created when drawing.
}

FontManager::FontManager() {
  // Add the test dummy font with empty name (equal to ID 0).
  AddFontInfo("-test-font-dummy-", "");
  m_test_font_desc.SetSize(16);
  CreateFontFace(m_test_font_desc);

  // Use the test dummy font as default by default.
  m_default_font_desc = m_test_font_desc;
}

FontManager::~FontManager() = default;

FontInfo* FontManager::AddFontInfo(const char* filename, const char* name) {
  FontInfo* fi = new FontInfo(filename, name);
  m_font_info.Add(fi->id(), fi);
  return fi;
}

FontInfo* FontManager::GetFontInfo(const TBID& id) const {
  return m_font_info.Get(id);
}

bool FontManager::HasFontFace(const FontDescription& font_desc) const {
  return m_fonts.Get(font_desc.GetFontFaceID()) ? true : false;
}

FontFace* FontManager::GetFontFace(const FontDescription& font_desc) {
  if (FontFace* font = m_fonts.Get(font_desc.GetFontFaceID())) {
    return font;
  }
  if (FontFace* font =
          m_fonts.Get(GetDefaultFontDescription().GetFontFaceID())) {
    return font;
  }
  return m_fonts.Get(m_test_font_desc.GetFontFaceID());
}

FontFace* FontManager::CreateFontFace(const FontDescription& font_desc) {
  assert(
      !HasFontFace(
          font_desc));  // There is already a font added with this description!

  FontInfo* fi = GetFontInfo(font_desc.id());
  if (!fi) {
    return nullptr;
  }

  if (fi->id() == 0) {
    // Is this the test dummy font.
    FontFace* font = new FontFace(&m_glyph_cache, nullptr, font_desc);
    m_fonts.Add(font_desc.GetFontFaceID(), font);
    return font;
  }

  // Iterate through font renderers until we find one capable of creating a font
  // for this file.
  for (FontRenderer* fr = m_font_renderers.GetFirst(); fr; fr = fr->GetNext()) {
    if (FontFace* font = fr->Create(this, fi->GetFilename(), font_desc)) {
      m_fonts.Add(font_desc.GetFontFaceID(), font);
      return font;
    }
  }
  return nullptr;
}

}  // namespace resources
}  // namespace tb
