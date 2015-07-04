/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_renderer.h"
#include "tb/resources/font_renderer.h"
#include "tb/util/file.h"

#ifdef TB_FONT_RENDERER_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

int num_fonts = 0;
bool ft_initialized = false;
static FT_Library g_freetype = nullptr;

using namespace tb;
using namespace tb::resources;

/** Cache of truetype file data, so it isn't loaded multiple times for each font
 * size */

class FreetypeFace;
static util::HashTableOf<FreetypeFace> ft_face_cache;

class FreetypeFace {
 public:
  FreetypeFace() : hashID(0), ttf_buffer(nullptr), m_face(0), refCount(1) {}
  ~FreetypeFace() {
    if (hashID) ft_face_cache.Remove(hashID);
    FT_Done_Face(m_face);
    delete[] ttf_buffer;
  }
  void Release() {
    --refCount;
    if (!refCount) delete this;
  }

  uint32_t hashID;
  unsigned char* ttf_buffer;
  FT_Face m_face;
  unsigned int refCount;
};

/** FreetypeFontRenderer renders fonts using the freetype library. */
class FreetypeFontRenderer : public FontRenderer {
 public:
  FreetypeFontRenderer();
  ~FreetypeFontRenderer();

  virtual FontFace* Create(FontManager* font_manager,
                           const std::string& filename,
                           const FontDescription& font_desc);

  virtual FontMetrics GetMetrics();
  virtual bool RenderGlyph(FontGlyphData* dst_bitmap, UCS4 cp);
  virtual void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp);

 private:
  bool Load(FreetypeFace* face, int size);
  bool Load(const char* filename, int size);

  FT_Size m_size;
  FreetypeFace* m_face;
};

FreetypeFontRenderer::FreetypeFontRenderer()
    : m_size(nullptr), m_face(nullptr) {
  num_fonts++;
}

FreetypeFontRenderer::~FreetypeFontRenderer() {
  FT_Done_Size(m_size);
  if (m_face) m_face->Release();

  num_fonts--;
  if (num_fonts == 0 && ft_initialized) {
    FT_Done_FreeType(g_freetype);
    ft_initialized = false;
  }
}

FontMetrics FreetypeFontRenderer::GetMetrics() {
  FontMetrics metrics;
  metrics.ascent = (int16_t)(m_size->metrics.ascender >> 6);
  metrics.descent = (int16_t) - (m_size->metrics.descender >> 6);
  metrics.height = (int16_t)(m_size->metrics.height >> 6);
  return metrics;
}

bool FreetypeFontRenderer::RenderGlyph(FontGlyphData* data, UCS4 cp) {
  FT_Activate_Size(m_size);
  FT_GlyphSlot slot = m_face->m_face->glyph;
  if (FT_Load_Char(m_face->m_face, cp, FT_LOAD_RENDER) ||
      slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
    return false;
  data->w = slot->bitmap.width;
  data->h = slot->bitmap.rows;
  data->stride = slot->bitmap.pitch;
  data->data8 = slot->bitmap.buffer;
  return data->data8 ? true : false;
}

void FreetypeFontRenderer::GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) {
  FT_Activate_Size(m_size);
  FT_GlyphSlot slot = m_face->m_face->glyph;
  if (FT_Load_Char(m_face->m_face, cp, FT_LOAD_RENDER)) return;
  metrics->advance = (int16_t)(slot->advance.x >> 6);
  metrics->x = slot->bitmap_left;
  metrics->y = -slot->bitmap_top;
}

bool FreetypeFontRenderer::Load(FreetypeFace* face, int size) {
  // Should not be possible to have a face if freetype is not initialized
  assert(ft_initialized);
  m_face = face;
  if (FT_New_Size(m_face->m_face, &m_size) || FT_Activate_Size(m_size) ||
      FT_Set_Pixel_Sizes(m_face->m_face, 0, size))
    return false;
  return true;
}

bool FreetypeFontRenderer::Load(const std::string& filename, int size) {
  if (!ft_initialized) ft_initialized = !FT_Init_FreeType(&g_freetype);
  if (!ft_initialized) return false;

  m_face = new FreetypeFace();

  auto file = util::File::Open(filename, util::File::Mode::kRead);
  if (!file) return false;

  size_t ttf_buf_size = file->Size();
  m_face->ttf_buffer = new unsigned char[ttf_buf_size];
  ttf_buf_size = file->Read(m_face->ttf_buffer, 1, ttf_buf_size);

  if (FT_New_Memory_Face(g_freetype, m_face->ttf_buffer, ttf_buf_size, 0,
                         &m_face->m_face))
    return false;
  return Load(m_face, size);
}

FontFace* FreetypeFontRenderer::Create(FontManager* font_manager,
                                       const std::string& filename,
                                       const FontDescription& font_desc) {
  if (FreetypeFontRenderer* fr = new FreetypeFontRenderer()) {
    TBID face_cache_id(filename);
    FreetypeFace* f = ft_face_cache.Get(face_cache_id);
    if (f) {
      ++f->refCount;
      if (fr->Load(f, (int)font_desc.GetSize()))
        if (FontFace* font =
                new FontFace(font_manager->GetGlyphCache(), fr, font_desc))
          return font;
    } else if (fr->Load(filename, (int)font_desc.GetSize())) {
      if (ft_face_cache.Add(face_cache_id, fr->m_face))
        fr->m_face->hashID = face_cache_id;
      if (FontFace* font =
              new FontFace(font_manager->GetGlyphCache(), fr, font_desc))
        return font;
    }

    delete fr;
  }
  return nullptr;
}

void register_freetype_font_renderer() {
  FreetypeFontRenderer* fr = new FreetypeFontRenderer();
  FontManager::get()->AddRenderer(fr);
}

#endif  // TB_FONT_RENDERER_FREETYPE
