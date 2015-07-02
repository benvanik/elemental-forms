/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_IMAGE_WIDGET_H
#define TB_IMAGE_WIDGET_H

#include "tb_image_manager.h"
#include "tb_widgets.h"

namespace tb {

// A widget showing a image loaded by ImageManager, constrained in size to its
// skin.
// If you need to show a image from the skin, you can use TBSkinImage.
class ImageWidget : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(ImageWidget, TBWidget);

  ImageWidget() = default;

  void SetImage(const Image& image) { m_image = image; }
  void SetImage(const char* filename) {
    m_image = g_image_manager->GetImage(filename);
  }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const INFLATE_INFO& info) override;
  void OnPaint(const PaintProps& paint_props) override;

 private:
  Image m_image;
};

}  // namespace tb

#endif  // TB_IMAGE_WIDGET_H
