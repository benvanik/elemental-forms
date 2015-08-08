/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_FONT_EFFECT_H_
#define EL_TEXT_FONT_EFFECT_H_

#include "el/util/string_builder.h"

namespace el {
namespace text {

class FontGlyphData;
class GlyphMetrics;

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
  util::StringBuilder m_blur_temp;
};

}  // namespace text
}  // namespace el

#endif  // EL_TEXT_FONT_EFFECT_H_
