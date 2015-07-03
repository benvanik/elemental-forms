/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_FONT_RENDERER_H
#define TB_FONT_RENDERER_H

#include "tb_core.h"
#include "tb_bitmap_fragment.h"
#include "tb_font_desc.h"
#include "tb_linklist.h"
#include "tb_renderer.h"
#include "tb_string_builder.h"
#include "utf8.h"

namespace tb {

class FontFace;

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

// Renders glyphs from a font file.
class FontRenderer : public TBLinkOf<FontRenderer> {
 public:
  virtual ~FontRenderer() = default;

  // Opens the given font file with this renderer and return a new FontFace with
  // it.
  // return nullptr if the file can't be opened by this renderer.
  virtual FontFace* Create(FontManager* font_manager,
                           const std::string& filename,
                           const FontDescription& font_desc) = 0;

  virtual bool RenderGlyph(FontGlyphData* data, UCS4 cp) = 0;
  virtual void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) = 0;
  virtual FontMetrics GetMetrics() = 0;
};

// Holds glyph metrics and bitmap fragment.
// There's one of these for all rendered (both successful and missing) glyphs in
// FontFace.
class FontGlyph : public TBLinkOf<FontGlyph> {
 public:
  FontGlyph(const TBID& hash_id, UCS4 cp);
  TBID hash_id;
  UCS4 cp;
  GlyphMetrics metrics;  // The glyph metrics.
  BitmapFragment* frag =
      nullptr;           // The bitmap fragment, or nullptr if missing.
  bool has_rgb = false;  // if true, drawing should ignore text color.
};

// Caches glyphs for font faces.
// Rendered glyphs use bitmap fragments from its fragment manager.
class FontGlyphCache : private RendererListener {
 public:
  FontGlyphCache();
  ~FontGlyphCache();

  // Gets the glyph or nullptr if it is not in the cache.
  FontGlyph* GetGlyph(const TBID& hash_id, UCS4 cp);

  // Creates the glyph and put it in the cache.
  // Returns the glyph, or nullptr on fail.
  FontGlyph* CreateAndCacheGlyph(const TBID& hash_id, UCS4 cp);

  // Creates a bitmap fragment for the given glyph and render data. This may
  // drop other rendered glyphs from the fragment map.
  // Returns the fragment, or nullptr on fail.
  BitmapFragment* CreateFragment(FontGlyph* glyph, int w, int h, int stride,
                                 uint32_t* data);

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the glyph bitmaps on screen, to analyze fragment positioning.
  void Debug();
#endif

  void OnContextLost() override;
  void OnContextRestored() override;

 private:
  void DropGlyphFragment(FontGlyph* glyph);

  BitmapFragmentManager m_frag_manager;
  TBHashTableAutoDeleteOf<FontGlyph> m_glyphs;
  TBLinkListOf<FontGlyph> m_all_rendered_glyphs;
};

// Applies an effect on each glyph that is rendered in a FontFace.
class FontEffect {
 public:
  FontEffect();
  ~FontEffect();

  // Sets blur radius. 0 means no blur.
  void SetBlurRadius(int blur_radius);

  // Returns true if the result is in RGB and should not be painted using the
  // color parameter given to DrawString. In other words: It's a color glyph.
  bool RendersInRGB() const { return false; }

  FontGlyphData* Render(GlyphMetrics* metrics, const FontGlyphData* src);

 private:
  int m_blur_radius = 0;
  float* m_tempBuffer = nullptr;
  float* m_kernel = nullptr;
  StringBuilder m_blur_temp;
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
#endif

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
  StringBuilder m_temp_buffer;

  FontFace* m_bgFont = nullptr;
  int m_bgX = 0;
  int m_bgY = 0;
  Color m_bgColor;
};

// Provides information about a font file associated with a font id.
class FontInfo {
 public:
  const std::string& GetFilename() const { return m_filename; }
  const std::string& GetName() const { return m_name; }

  // Gets the font ID that can be used to create this font from a
  // FontDescription (See FontDescription::SetID).
  TBID GetID() const { return m_id; }

 private:
  friend class FontManager;
  FontInfo(const char* filename, const char* name)
      : m_filename(filename), m_name(name), m_id(name) {}
  std::string m_filename;
  std::string m_name;
  TBID m_id;
};

// Creates and owns font faces (FontFace) which are looked up from
// FontDescription using GetFontFace.
// The fonts it can return must first have their file added and indexed
// (AddFontInfo), and then created CreateFontFace. Otherwise when asking for a
// font and it doesn't exist, it will use the default font.
// Font ID 0 is always populated with a dummy font that draws squares. This font
// is generally not used for other things than unit testing or as fallback when
// there is no font backend implemented yet. Since there is always at least the
// test font, no nullptr checks are needed.
class FontManager {
 public:
  FontManager();
  ~FontManager();

  // Adds a renderer so fonts supported by the renderer can be created.
  // Ownership of the renderer is taken, until calling RemoveRenderer.
  void AddRenderer(FontRenderer* renderer) {
    m_font_renderers.AddLast(renderer);
  }
  void RemoveRenderer(FontRenderer* renderer) {
    m_font_renderers.Remove(renderer);
  }

  // Adds FontInfo for the given font filename, so it can be loaded and
  // identified using the font id in a FontDescription.
  FontInfo* AddFontInfo(const char* filename, const char* name);

  // Gets FontInfo for the given font id, or nullptr if there is no match.
  FontInfo* GetFontInfo(const TBID& id) const;

  // Returns true if there is a font loaded that match the given font
  // description.
  bool HasFontFace(const FontDescription& font_desc) const;

  // Gets a loaded font matching the description, or the default font if there
  // is no exact match.
  // If there is not even any default font loaded, it will return the test dummy
  // font (rendering only squares).
  FontFace* GetFontFace(const FontDescription& font_desc);

  // Create and add a font with the given description.
  // Returns the created font face, or nullptr on fail. The font is owned by
  // this FontManager, and can be recieved from GetFontFace using the same
  // FontDescription.
  FontFace* CreateFontFace(const FontDescription& font_desc);

  // Sets the default font description. This is the font description that will
  // be used by default for elements. By default, the default description is
  // using the test dummy font.
  void SetDefaultFontDescription(const FontDescription& font_desc) {
    m_default_font_desc = font_desc;
  }
  FontDescription GetDefaultFontDescription() const {
    return m_default_font_desc;
  }

  // Returns the glyph cache used for fonts created by this font manager.
  FontGlyphCache* GetGlyphCache() { return &m_glyph_cache; }

 private:
  TBHashTableAutoDeleteOf<FontInfo> m_font_info;
  TBHashTableAutoDeleteOf<FontFace> m_fonts;
  TBLinkListAutoDeleteOf<FontRenderer> m_font_renderers;
  FontGlyphCache m_glyph_cache;
  FontDescription m_default_font_desc;
  FontDescription m_test_font_desc;
};

}  // namespace tb

#endif  // TB_FONT_RENDERER_H
