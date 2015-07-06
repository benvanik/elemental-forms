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
  return PreferredSize(m_image.width(), m_image.height());
}

void ImageBox::OnPaint(const PaintProps& paint_props) {
  if (auto fragment = m_image.bitmap()) {
    graphics::Renderer::get()->DrawBitmap(
        padding_rect(), Rect(0, 0, m_image.width(), m_image.height()),
        fragment);
  }
}

}  // namespace elements
}  // namespace el
