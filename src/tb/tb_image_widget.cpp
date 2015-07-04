/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_image_widget.h"
#include "tb_node_tree.h"

#include "tb/resources/element_factory.h"

namespace tb {

void ImageElement::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(ImageElement, Value::Type::kNull,
                               ElementZ::kTop);
}

void ImageElement::OnInflate(const resources::InflateInfo& info) {
  if (const char* filename = info.node->GetValueString("filename", nullptr)) {
    SetImage(filename);
  }
  Element::OnInflate(info);
}

PreferredSize ImageElement::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  return PreferredSize(m_image.Width(), m_image.Height());
}

void ImageElement::OnPaint(const PaintProps& paint_props) {
  if (BitmapFragment* fragment = m_image.GetBitmap()) {
    Renderer::get()->DrawBitmap(GetPaddingRect(),
                                Rect(0, 0, m_image.Width(), m_image.Height()),
                                fragment);
  }
}

}  // namespace tb
