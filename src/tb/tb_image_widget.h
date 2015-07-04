/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_IMAGE_WIDGET_H
#define TB_IMAGE_WIDGET_H

#include "tb_image_manager.h"
#include "tb_widgets.h"

namespace tb {

// A element showing a image loaded by ImageManager, constrained in size to its
// skin.
// If you need to show a image from the skin, you can use SkinImage.
class ImageElement : public Element {
 public:
  TBOBJECT_SUBCLASS(ImageElement, Element);

  ImageElement() = default;

  void SetImage(const Image& image) { m_image = image; }
  void SetImage(const char* filename) {
    m_image = ImageManager::get()->GetImage(filename);
  }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const InflateInfo& info) override;
  void OnPaint(const PaintProps& paint_props) override;

 private:
  Image m_image;
};

}  // namespace tb

#endif  // TB_IMAGE_WIDGET_H
