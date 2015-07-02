/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_image_widget.h"

#include "tb_node_tree.h"
#include "tb_widgets_reader.h"

namespace tb {

PreferredSize TBImageWidget::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  return PreferredSize(m_image.Width(), m_image.Height());
}

void TBImageWidget::OnPaint(const PaintProps& paint_props) {
  if (TBBitmapFragment* fragment = m_image.GetBitmap())
    g_renderer->DrawBitmap(GetPaddingRect(),
                           Rect(0, 0, m_image.Width(), m_image.Height()),
                           fragment);
}

}  // namespace tb
