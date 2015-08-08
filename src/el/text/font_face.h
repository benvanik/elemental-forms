/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_FONT_FACE_H_
#define EL_TEXT_FONT_FACE_H_

#include <memory>
#include <string>

#include "el/color.h"
#include "el/font_description.h"
#include "el/text/font_effect.h"
#include "el/text/utf8.h"
#include "el/util/intrusive_list.h"
#include "el/util/string_builder.h"

namespace el {
namespace graphics {
class BitmapFragment;
}  // namespace graphics
}  // namespace el

namespace el {
namespace text {

class FontGlyphCache;
class FontRenderer;

// Rendering info used during glyph rendering by FontRenderer.
// It does not own the data pointers.
class FontGlyphData {
 public:
  uint8_t* data8 = nullptr;
  uint32_t* data32 = nullptr;
  int w = 0;
  int h = 0;
  int stride = 0;
  bool rgb = false;
};

// Contains metrics for a font glyph.
class GlyphMetrics {
 public:
  int16_t advance = 0;
  int16_t x = 0;
  int16_t y = 0;
};

// Contains metrics for a font face.
class FontMetrics {
 public:
  int16_t ascent = 0;   // Ascent. See FontFace::ascent().
  int16_t descent = 0;  // Descent. See FontFace::descent().
  int16_t height = 0;   // Height. See FontFace::height().
};

// Holds glyph metrics and bitmap fragment.
// There's one of these for all rendered (both successful and missing) glyphs in
// FontFace.
class FontGlyph : public util::IntrusiveListEntry<FontGlyph> {
 public:
  FontGlyph(const TBID& hash_id, utf8::UCS4 cp);
  TBID hash_id;
  utf8::UCS4 cp;
  GlyphMetrics metrics;  // The glyph metrics.
  graphics::BitmapFragment* frag =
      nullptr;           // The bitmap fragment, or nullptr if missing.
  bool has_rgb = false;  // if true, drawing should ignore text color.
};

// Represents a loaded font that can measure and render strings.
class FontFace {
 public:
  FontFace(FontGlyphCache* glyph_cache, std::unique_ptr<FontRenderer> renderer,
           const FontDescription& font_desc);
  ~FontFace();

  // Renders all glyphs needed to display the string.
  bool RenderGlyphs(const char* glyph_str,
                    size_t glyph_str_len = std::string::npos);

  // Gets the vertical distance (positive) from the horizontal baseline to the
  // highest character coordinate in a font face.
  int ascent() const { return m_metrics.ascent; }

  // Gets the vertical distance (positive) from the horizontal baseline to the
  // lowest character coordinate in the font face.
  int descent() const { return m_metrics.descent; }

  // Gets height of the font in pixels.
  int height() const { return m_metrics.height; }

  // Gets the font description that was used to create this font.
  FontDescription font_description() const { return m_font_desc; }

  // Gets the effect object, so the effect can be changed.
  // NOTE: No glyphs are re-rendered. Only new glyphs are affected.
  FontEffect* effect() { return &m_effect; }

  // Draw string at position x, y (marks the upper left corner of the text).
  void DrawString(int x, int y, const Color& color, const char* str,
                  size_t len = std::string::npos);
  void DrawString(int x, int y, const Color& color, const std::string& str,
                  size_t len = std::string::npos) {
    DrawString(x, y, color, str.c_str(), len);
  }

  // Measures the width of the given string. Should measure len characters or to
  // the null termination (whatever comes first).
  int GetStringWidth(const char* str, size_t len = std::string::npos);
  int GetStringWidth(const std::string& str, size_t len = std::string::npos) {
    return GetStringWidth(str.c_str(), len);
  }

#ifdef EL_RUNTIME_DEBUG_INFO
  // Renders the glyph bitmaps on screen, to analyze fragment positioning.
  void Debug();
#endif  // EL_RUNTIME_DEBUG_INFO

  // Sets a background font which will always be rendered behind this one when
  // calling DrawString. Useful to add a shadow effect to a font.
  void SetBackgroundFont(FontFace* font, const Color& col, int xofs, int yofs);

 private:
  TBID GetHashId(utf8::UCS4 cp) const;
  FontGlyph* GetGlyph(utf8::UCS4 cp, bool render_if_needed);
  FontGlyph* CreateAndCacheGlyph(utf8::UCS4 cp);
  void RenderGlyph(FontGlyph* glyph);

  FontGlyphCache* m_glyph_cache = nullptr;
  std::unique_ptr<FontRenderer> m_font_renderer;
  FontDescription m_font_desc;
  FontMetrics m_metrics;
  FontEffect m_effect;
  util::StringBuilder m_temp_buffer;

  FontFace* m_bgFont = nullptr;
  int m_bgX = 0;
  int m_bgY = 0;
  Color m_bgColor;
};

}  // namespace text
}  // namespace el

#endif  // EL_TEXT_FONT_FACE_H_
