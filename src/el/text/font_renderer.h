/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_FONT_RENDERER_H_
#define EL_TEXT_FONT_RENDERER_H_

#include <memory>
#include <string>

#include "el/font_description.h"
#include "el/graphics/renderer.h"
#include "el/text/utf8.h"

namespace el {
namespace text {

class FontFace;
class FontGlyphData;
class FontManager;
class FontMetrics;
class GlyphMetrics;

// Renders glyphs from a font file.
class FontRenderer {
 public:
  virtual ~FontRenderer() = default;

  // Opens the given font file with this renderer and return a new FontFace with
  // it.
  // Returns nullptr if the file can't be opened by this renderer.
  virtual std::unique_ptr<FontFace> Create(
      FontManager* font_manager, const std::string& filename,
      const FontDescription& font_desc) = 0;

  virtual bool RenderGlyph(FontGlyphData* data, utf8::UCS4 cp) = 0;
  virtual void GetGlyphMetrics(GlyphMetrics* metrics, utf8::UCS4 cp) = 0;
  virtual FontMetrics GetMetrics() = 0;
};

}  // namespace text
}  // namespace el

#endif  // EL_TEXT_FONT_RENDERER_H_
