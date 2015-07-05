/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_IMAGE_BOX_H_
#define TB_ELEMENTS_IMAGE_BOX_H_

#include "tb/element.h"
#include "tb/graphics/image_manager.h"

namespace tb {
namespace elements {

// A element showing a image loaded by ImageManager, constrained in size to its
// skin.
// If you need to show a image from the skin, you can use SkinImage.
class ImageBox : public Element {
 public:
  TBOBJECT_SUBCLASS(ImageBox, Element);
  static void RegisterInflater();

  ImageBox() = default;

  void SetImage(const graphics::Image& image) { m_image = image; }
  void SetImage(const char* filename) {
    m_image = graphics::ImageManager::get()->GetImage(filename);
  }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnPaint(const PaintProps& paint_props) override;

 private:
  graphics::Image m_image;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_IMAGE_BOX_H_
