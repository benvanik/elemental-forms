/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_FONT_RENDERER_H_
#define TB_RESOURCES_FONT_RENDERER_H_

#include <string>

#include "tb/font_description.h"
#include "tb/graphics/renderer.h"
#include "tb/util/utf8.h"

namespace tb {
namespace resources {

class FontFace;
class FontGlyphData;
class FontManager;
class FontMetrics;
class GlyphMetrics;
namespace {
using UCS4 = tb::util::utf8::UCS4;
}  // namespace

// Renders glyphs from a font file.
class FontRenderer : public util::TBLinkOf<FontRenderer> {
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

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_FONT_RENDERER_H_