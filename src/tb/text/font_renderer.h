/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TEXT_FONT_RENDERER_H_
#define TB_TEXT_FONT_RENDERER_H_

#include <memory>
#include <string>

#include "tb/font_description.h"
#include "tb/graphics/renderer.h"
#include "tb/text/utf8.h"

namespace tb {
namespace text {

class FontFace;
class FontGlyphData;
class FontManager;
class FontMetrics;
class GlyphMetrics;
namespace {
using UCS4 = tb::text::utf8::UCS4;
}  // namespace

// Renders glyphs from a font file.
class FontRenderer : public util::TBLinkOf<FontRenderer> {
 public:
  virtual ~FontRenderer() = default;

  // Opens the given font file with this renderer and return a new FontFace with
  // it.
  // Returns nullptr if the file can't be opened by this renderer.
  virtual std::unique_ptr<FontFace> Create(
      FontManager* font_manager, const std::string& filename,
      const FontDescription& font_desc) = 0;

  virtual bool RenderGlyph(FontGlyphData* data, UCS4 cp) = 0;
  virtual void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) = 0;
  virtual FontMetrics GetMetrics() = 0;
};

}  // namespace text
}  // namespace tb

#endif  // TB_TEXT_FONT_RENDERER_H_
