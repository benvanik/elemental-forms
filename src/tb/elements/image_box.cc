/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/image_box.h"
#include "tb/parsing/element_inflater.h"
#include "tb/parsing/parse_node.h"

namespace tb {
namespace elements {

void ImageBox::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(ImageBox, Value::Type::kNull, ElementZ::kTop);
}

void ImageBox::OnInflate(const parsing::InflateInfo& info) {
  if (const char* filename = info.node->GetValueString("filename", nullptr)) {
    SetImage(filename);
  }
  Element::OnInflate(info);
}

PreferredSize ImageBox::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  return PreferredSize(m_image.Width(), m_image.Height());
}

void ImageBox::OnPaint(const PaintProps& paint_props) {
  if (auto fragment = m_image.GetBitmap()) {
    graphics::Renderer::get()->DrawBitmap(
        GetPaddingRect(), Rect(0, 0, m_image.Width(), m_image.Height()),
        fragment);
  }
}

}  // namespace elements
}  // namespace tb
