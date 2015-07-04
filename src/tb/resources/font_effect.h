/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_FONT_EFFECT_H_
#define TB_RESOURCES_FONT_EFFECT_H_

#include "tb/util/string_builder.h"

namespace tb {
namespace resources {

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

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_FONT_EFFECT_H_
