/**
******************************************************************************
* xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
******************************************************************************
* Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
* See turbo_badger.h and LICENSE in the root for more information.           *
******************************************************************************
*/

#ifndef TB_RESOURCES_FONT_FACE_H_
#define TB_RESOURCES_FONT_FACE_H_

#include <memory>

#include "tb/color.h"
#include "tb/font_description.h"
#include "tb/resources/font_effect.h"
#include "tb/util/link_list.h"
#include "tb/util/string_builder.h"
#include "tb/util/utf8.h"

namespace tb {
namespace resources {

class BitmapFragment;
class FontGlyphCache;
class FontRenderer;
namespace {
using UCS4 = tb::util::utf8::UCS4;
}  // namespace

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
  int16_t ascent = 0;   // Ascent. See FontFace::GetAscent().
  int16_t descent = 0;  // Descent. See FontFace::GetDescent().
  int16_t height = 0;   // Height. See FontFace::height().
};

// Holds glyph metrics and bitmap fragment.
// There's one of these for all rendered (both successful and missing) glyphs in
// FontFace.
class FontGlyph : public util::TBLinkOf<FontGlyph> {
 public:
  FontGlyph(const TBID& hash_id, UCS4 cp);
  TBID hash_id;
  UCS4 cp;
  GlyphMetrics metrics;  // The glyph metrics.
  BitmapFragment* frag =
      nullptr;           // The bitmap fragment, or nullptr if missing.
  bool has_rgb = false;  // if true, drawing should ignore text color.
};

// Represents a loaded font that can measure and render strings.
class FontFace {
 public:
  FontFace(FontGlyphCache* glyph_cache, FontRenderer* renderer,
           const FontDescription& font_desc);
  ~FontFace();

  // Renders all glyphs needed to display the string.
  bool RenderGlyphs(const char* glyph_str,
                    size_t glyph_str_len = std::string::npos);

  // Gets the vertical distance (positive) from the horizontal baseline to the
  // highest character coordinate in a font face.
  int GetAscent() const { return m_metrics.ascent; }

  // Gets the vertical distance (positive) from the horizontal baseline to the
  // lowest character coordinate in the font face.
  int GetDescent() const { return m_metrics.descent; }

  // Gets height of the font in pixels.
  int height() const { return m_metrics.height; }

  // Gets the font description that was used to create this font.
  FontDescription GetFontDescription() const { return m_font_desc; }

  // Gets the effect object, so the effect can be changed.
  // NOTE: No glyphs are re-rendered. Only new glyphs are affected.
  FontEffect* GetEffect() { return &m_effect; }

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

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the glyph bitmaps on screen, to analyze fragment positioning.
  void Debug();
#endif  // TB_RUNTIME_DEBUG_INFO

  // Sets a background font which will always be rendered behind this one when
  // calling DrawString. Useful to add a shadow effect to a font.
  void SetBackgroundFont(FontFace* font, const Color& col, int xofs, int yofs);

 private:
  TBID GetHashId(UCS4 cp) const;
  FontGlyph* GetGlyph(UCS4 cp, bool render_if_needed);
  FontGlyph* CreateAndCacheGlyph(UCS4 cp);
  void RenderGlyph(FontGlyph* glyph);

  FontGlyphCache* m_glyph_cache = nullptr;
  FontRenderer* m_font_renderer = nullptr;
  FontDescription m_font_desc;
  FontMetrics m_metrics;
  FontEffect m_effect;
  util::StringBuilder m_temp_buffer;

  FontFace* m_bgFont = nullptr;
  int m_bgX = 0;
  int m_bgY = 0;
  Color m_bgColor;
};

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_FONT_FACE_H_
