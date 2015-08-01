/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cmath>
#include <cstring>
#include <memory>

#include "el/text/font_face.h"
#include "el/text/font_manager.h"
#include "el/text/font_renderer.h"
#include "el/text/utf8.h"

namespace el {
namespace text {

using graphics::Renderer;

FontGlyph::FontGlyph(const TBID& hash_id, UCS4 cp) : hash_id(hash_id), cp(cp) {}

FontFace::FontFace(FontGlyphCache* glyph_cache,
                   std::unique_ptr<FontRenderer> renderer,
                   const FontDescription& font_desc)
    : m_glyph_cache(glyph_cache),
      m_font_renderer(std::move(renderer)),
      m_font_desc(font_desc) {
  if (m_font_renderer) {
    m_metrics = m_font_renderer->GetMetrics();
  } else {
    // Invent some metrics for the test font.
    int16_t size = m_font_desc.size();
    m_metrics.ascent = size - size / 4;
    m_metrics.descent = size / 4;
    m_metrics.height = size;
  }
}

FontFace::~FontFace() {
  // It would be nice to drop all glyphs we have live for this font face.
  // Now they only die when they get old and kicked out of the cache.
  // We currently don't drop any font faces either though (except on shutdown)
  m_font_renderer.reset();
}

void FontFace::SetBackgroundFont(FontFace* font, const Color& col, int xofs,
                                 int yofs) {
  m_bgFont = font;
  m_bgX = xofs;
  m_bgY = yofs;
  m_bgColor = col;
}

bool FontFace::RenderGlyphs(const char* glyph_str, size_t glyph_str_len) {
  if (!m_font_renderer) {
    // This is the test font.
    return true;
  }

  if (glyph_str_len == std::string::npos) {
    glyph_str_len = strlen(glyph_str);
  }

  bool has_all_glyphs = true;
  size_t i = 0;
  while (glyph_str[i] && i < glyph_str_len) {
    UCS4 cp = utf8::decode_next(glyph_str, &i, glyph_str_len);
    if (!GetGlyph(cp, true)) {
      has_all_glyphs = false;
    }
  }
  return has_all_glyphs;
}

FontGlyph* FontFace::CreateAndCacheGlyph(UCS4 cp) {
  if (!m_font_renderer) {
    // This is the test font.
    return nullptr;
  }

  // Create the new glyph.
  FontGlyph* glyph = m_glyph_cache->CreateAndCacheGlyph(GetHashId(cp), cp);
  if (glyph) m_font_renderer->GetGlyphMetrics(&glyph->metrics, cp);
  return glyph;
}

void FontFace::RenderGlyph(FontGlyph* glyph) {
  assert(!glyph->frag);
  FontGlyphData glyph_data;
  if (m_font_renderer->RenderGlyph(&glyph_data, glyph->cp)) {
    FontGlyphData* effect_glyph_data =
        m_effect.Render(&glyph->metrics, &glyph_data);
    FontGlyphData* result_glyph_data =
        effect_glyph_data ? effect_glyph_data : &glyph_data;

    // The glyph data may be in uint8_t format, which we have to convert since
    // we always create fragments (and Bitmap) in 32bit format.
    uint32_t* glyph_dsta_src = result_glyph_data->data32;
    if (!glyph_dsta_src && result_glyph_data->data8) {
      m_temp_buffer.Reserve(result_glyph_data->w * result_glyph_data->h *
                            sizeof(uint32_t));
      glyph_dsta_src = (uint32_t*)m_temp_buffer.data();
      for (int y = 0; y < result_glyph_data->h; y++) {
        for (int x = 0; x < result_glyph_data->w; x++) {
          glyph_dsta_src[x + y * result_glyph_data->w] = Color(
              255, 255, 255,
              result_glyph_data->data8[x + y * result_glyph_data->stride]);
        }
      }
    }

    // Finally, the glyph data is ready and we can create a bitmap fragment.
    if (glyph_dsta_src) {
      glyph->has_rgb = result_glyph_data->rgb;
      m_glyph_cache->CreateFragment(glyph, result_glyph_data->w,
                                    result_glyph_data->h,
                                    result_glyph_data->stride, glyph_dsta_src);
    }

    delete effect_glyph_data;
  }
#ifdef EL_RUNTIME_DEBUG_INFO
// char glyph_str[9];
// int len = utf8::encode(cp, glyph_str);
// glyph_str[len] = 0;
// std::string info;
// info.SetFormatted("Created glyph %d (\"%s\"). Cache contains %d glyphs (%d%%
// full) using %d bitmaps.\n", cp, glyph_str, m_all_glyphs.CountLinks(),
// m_frag_manager.GetUseRatio(), m_frag_manager.map_count());
// TBDebugOut(info);
#endif  // EL_RUNTIME_DEBUG_INFO
}

TBID FontFace::GetHashId(UCS4 cp) const {
  return cp * 31 + m_font_desc.font_face_id();
}

FontGlyph* FontFace::GetGlyph(UCS4 cp, bool render_if_needed) {
  FontGlyph* glyph = m_glyph_cache->GetGlyph(GetHashId(cp), cp);
  if (!glyph) {
    glyph = CreateAndCacheGlyph(cp);
  }
  if (glyph && !glyph->frag && render_if_needed) {
    RenderGlyph(glyph);
  }
  return glyph;
}

void FontFace::DrawString(int x, int y, const Color& color, const char* str,
                          size_t len) {
  if (m_bgFont) {
    m_bgFont->DrawString(x + m_bgX, y + m_bgY, m_bgColor, str, len);
  }

  if (m_font_renderer) {
    Renderer::get()->BeginBatchHint(Renderer::BatchHint::kDrawBitmapFragment);
  }

  size_t i = 0;
  while (str[i] && i < len) {
    UCS4 cp = utf8::decode_next(str, &i, len);
    if (cp == 0xFFFF) continue;
    if (FontGlyph* glyph = GetGlyph(cp, true)) {
      if (glyph->frag) {
        Rect dst_rect(x + glyph->metrics.x, y + glyph->metrics.y + ascent(),
                      glyph->frag->width(), glyph->frag->height());
        Rect src_rect(0, 0, glyph->frag->width(), glyph->frag->height());
        if (glyph->has_rgb) {
          Renderer::get()->DrawBitmap(dst_rect, src_rect, glyph->frag);
        } else {
          Renderer::get()->DrawBitmapColored(dst_rect, src_rect, color,
                                             glyph->frag);
        }
      }
      x += glyph->metrics.advance;
    } else if (!m_font_renderer) {
      // This is the test font. Use same glyph width as height and draw square.
      Renderer::get()->DrawRect(
          Rect(x, y, m_metrics.height / 3, m_metrics.height), color);
      x += m_metrics.height / 3 + 1;
    }
  }

  if (m_font_renderer) {
    Renderer::get()->EndBatchHint();
  }
}

int FontFace::GetStringWidth(const char* str, size_t len) {
  int width = 0;
  size_t i = 0;
  while (str[i] && i < len) {
    UCS4 cp = utf8::decode_next(str, &i, len);
    if (cp == 0xFFFF) {
      continue;
    }
    if (!m_font_renderer) {
      // This is the test font. Use same glyph width as height.
      width += m_metrics.height / 3 + 1;
    } else if (FontGlyph* glyph = GetGlyph(cp, false)) {
      width += glyph->metrics.advance;
    }
  }
  return width;
}

#ifdef EL_RUNTIME_DEBUG_INFO
void FontFace::Debug() { m_glyph_cache->Debug(); }
#endif  // EL_RUNTIME_DEBUG_INFO

}  // namespace text
}  // namespace el
