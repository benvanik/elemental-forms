/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/image_box.h"
#include "el/parsing/element_inflater.h"
#include "el/parsing/parse_node.h"

namespace el {
namespace elements {

void ImageBox::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(ImageBox, Value::Type::kNull, ElementZ::kTop);
}

void ImageBox::OnInflate(const parsing::InflateInfo& info) {
  if (const char* filename = info.node->GetValueString("filename", nullptr)) {
    set_image(filename);
  }
  Element::OnInflate(info);
}

PreferredSize ImageBox::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  if (bitmap_) {
    return PreferredSize(bitmap_->width(), bitmap_->height());
  } else {
    return PreferredSize(image_.width(), image_.height());
  }
}

void ImageBox::OnPaint(const PaintProps& paint_props) {
  if (bitmap_) {
    graphics::Renderer::get()->DrawBitmap(
        padding_rect(), Rect(0, 0, bitmap_->width(), bitmap_->height()),
        bitmap_);
  } else {
    if (auto fragment = image_.bitmap()) {
      graphics::Renderer::get()->DrawBitmap(
          padding_rect(), Rect(0, 0, image_.width(), image_.height()),
          fragment);
    }
  }
}

}  // namespace elements
}  // namespace el
