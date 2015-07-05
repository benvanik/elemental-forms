/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/graphics/renderer.h"
#include "tb/text/font_renderer.h"

#ifdef TB_FONT_RENDERER_STB

using namespace tb;
using namespace tb::resources;

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate
                                     // implementation
#include "thirdparty/stb_truetype.h"

/** SFontRenderer renders fonts using stb_truetype.h (http://nothings.org/) */

class SFontRenderer : public FontRenderer {
 public:
  SFontRenderer();
  ~SFontRenderer();

  bool Load(const char* filename, int size);

  virtual FontFace* Create(FontManager* font_manager,
                           const std::string& filename,
                           const FontDescription& font_desc);

  virtual FontMetrics GetMetrics();
  virtual bool RenderGlyph(FontGlyphData* dst_bitmap, UCS4 cp);
  virtual void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp);

 private:
  stbtt_fontinfo font;
  unsigned char* ttf_buffer;
  unsigned char* render_data;
  int font_size;
  float scale;
};

SFontRenderer::SFontRenderer() : ttf_buffer(nullptr), render_data(nullptr) {}

SFontRenderer::~SFontRenderer() {
  delete[] ttf_buffer;
  delete[] render_data;
}

FontMetrics SFontRenderer::GetMetrics() {
  FontMetrics metrics;
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
  metrics.ascent = (int)(ascent * scale + 0.5f);
  metrics.descent = (int)((-descent) * scale + 0.5f);
  metrics.height = (int)((ascent - descent + lineGap) * scale + 0.5f);
  return metrics;
}

bool SFontRenderer::RenderGlyph(FontGlyphData* data, UCS4 cp) {
  delete[] render_data;
  render_data =
      stbtt_GetCodepointBitmap(&font, 0, scale, cp, &data->w, &data->h, 0, 0);
  data->data8 = render_data;
  data->stride = data->w;
  data->rgb = false;
  return data->data8 ? true : false;
}

void SFontRenderer::GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) {
  int advanceWidth, leftSideBearing;
  stbtt_GetCodepointHMetrics(&font, cp, &advanceWidth, &leftSideBearing);
  metrics->advance = (int)(advanceWidth * scale + 0.5f);
  int ix0, iy0, ix1, iy1;
  stbtt_GetCodepointBitmapBox(&font, cp, 0, scale, &ix0, &iy0, &ix1, &iy1);
  metrics->x = ix0;
  metrics->y = iy0;
}

bool SFontRenderer::Load(const char* filename, int size) {
  auto file = util::File::Open(filename, util::File::Mode::kRead);
  if (!file) return false;

  size_t ttf_buf_size = file->Size();
  ttf_buffer = new unsigned char[ttf_buf_size];
  ttf_buf_size = file->Read(ttf_buffer, 1, ttf_buf_size);

  stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));

  font_size = (int)(size * 1.3f);  // FIX: Constant taken out of thin air
                                   // because fonts get too small.
  scale = stbtt_ScaleForPixelHeight(&font, (float)font_size);
  return true;
}

FontFace* SFontRenderer::Create(FontManager* font_manager, const char* filename,
                                const FontDescription& font_desc) {
  SFontRenderer* fr = new SFontRenderer();
  if (fr->Load(filename, (int)font_desc.size())) {
    FontFace* font = new FontFace(font_manager->glyph_cache(), fr, font_desc);
    return font;
  }
  delete fr;
  return nullptr;
}

void register_stb_font_renderer() {
  FontManager::get()->RegisterRenderer(std::make_unique<SFontRenderer>());
}

#endif  // TB_FONT_RENDERER_STB
