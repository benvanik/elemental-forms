/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TEXT_FONT_MANAGER_H_
#define TB_TEXT_FONT_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tb/font_description.h"
#include "tb/graphics/bitmap_fragment.h"
#include "tb/graphics/bitmap_fragment_manager.h"
#include "tb/graphics/renderer.h"
#include "tb/text/font_face.h"
#include "tb/text/font_renderer.h"
#include "tb/text/utf8.h"

namespace tb {
namespace text {

namespace {
using UCS4 = tb::text::utf8::UCS4;
}  // namespace

// Provides information about a font file associated with a font id.
class FontInfo {
 public:
  const std::string& filename() const { return m_filename; }
  const std::string& name() const { return m_name; }

  // Gets the font ID that can be used to create this font from a
  // FontDescription (See FontDescription::SetID).
  TBID id() const { return m_id; }

 private:
  friend class FontManager;
  FontInfo(const char* filename, const char* name)
      : m_filename(filename), m_name(name), m_id(name) {}
  std::string m_filename;
  std::string m_name;
  TBID m_id;
};

// Caches glyphs for font faces.
// Rendered glyphs use bitmap fragments from its fragment manager.
class FontGlyphCache : private graphics::RendererListener {
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
  graphics::BitmapFragment* CreateFragment(FontGlyph* glyph, int w, int h,
                                           int stride, uint32_t* data);

#ifdef TB_RUNTIME_DEBUG_INFO
  // Renders the glyph bitmaps on screen, to analyze fragment positioning.
  void Debug();
#endif

  void OnContextLost() override;
  void OnContextRestored() override;

 private:
  void DropGlyphFragment(FontGlyph* glyph);

  graphics::BitmapFragmentManager m_frag_manager;
  std::unordered_map<uint32_t, std::unique_ptr<FontGlyph>> m_glyphs;
  util::IntrusiveList<FontGlyph> m_all_rendered_glyphs;
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
  static FontManager* get() { return font_manager_singleton_.get(); }
  static void set(std::unique_ptr<FontManager> value) {
    font_manager_singleton_ = std::move(value);
  }

  FontManager();
  ~FontManager();

  // Adds a renderer so fonts supported by the renderer can be created.
  void RegisterRenderer(std::unique_ptr<FontRenderer> renderer) {
    m_font_renderers.emplace_back(std::move(renderer));
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

  FontDescription default_font_description() const {
    return m_default_font_desc;
  }
  // Sets the default font description. This is the font description that will
  // be used by default for elements. By default, the default description is
  // using the test dummy font.
  void set_default_font_description(const FontDescription& font_desc) {
    m_default_font_desc = font_desc;
  }

  // Returns the glyph cache used for fonts created by this font manager.
  FontGlyphCache* glyph_cache() { return &m_glyph_cache; }

 private:
  static std::unique_ptr<FontManager> font_manager_singleton_;

  std::unordered_map<uint32_t, std::unique_ptr<FontInfo>> m_font_info;
  std::unordered_map<uint32_t, std::unique_ptr<FontFace>> m_fonts;
  std::vector<std::unique_ptr<FontRenderer>> m_font_renderers;
  FontGlyphCache m_glyph_cache;
  FontDescription m_default_font_desc;
  FontDescription m_test_font_desc;
};

}  // namespace text
}  // namespace tb

#endif  // TB_TEXT_FONT_MANAGER_H_
