/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_IMAGE_BOX_H_
#define EL_ELEMENTS_IMAGE_BOX_H_

#include <string>

#include "el/element.h"
#include "el/graphics/image_manager.h"

namespace el {
namespace elements {

// A element showing a image loaded by ImageManager, constrained in size to its
// skin.
// If you need to show a image from the skin, you can use IconBox.
class ImageBox : public Element {
 public:
  TBOBJECT_SUBCLASS(ImageBox, Element);
  static void RegisterInflater();

  ImageBox() = default;

  void set_image(graphics::Bitmap* bitmap) {
    bitmap_ = bitmap;
    image_ = graphics::Image();
  }
  void set_image(const graphics::Image& image) {
    bitmap_ = nullptr;
    image_ = image;
  }
  void set_image(const char* filename) {
    set_image(graphics::ImageManager::get()->GetImage(filename));
  }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnPaint(const PaintProps& paint_props) override;

 private:
  graphics::Bitmap* bitmap_ = nullptr;
  graphics::Image image_;
};

}  // namespace elements
namespace dsl {

struct ImageBoxNode : public ElementNode<ImageBoxNode> {
  using R = ImageBoxNode;
  explicit ImageBoxNode(const char* filename = nullptr)
      : ElementNode("ImageBox") {
    if (filename) {
      this->filename(filename);
    }
  }
  //
  R& filename(std::string value) {
    set("filename", value);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_IMAGE_BOX_H_
