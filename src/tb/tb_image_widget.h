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

/** TBImageWidget is a widget showing a image loaded by TBImageManager,
        constrained in size to its skin.
        If you need to show a image from the skin, you can use TBSkinImage. */

class TBImageWidget : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(TBImageWidget, TBWidget);

  TBImageWidget() {}

  void SetImage(const TBImage& image) { m_image = image; }
  void SetImage(const char* filename) {
    m_image = g_image_manager->GetImage(filename);
  }

  virtual PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints);

  virtual void OnInflate(const INFLATE_INFO& info);
  virtual void OnPaint(const PaintProps& paint_props);

 private:
  TBImage m_image;
};

}  // namespace tb

#endif  // TB_IMAGE_WIDGET_H
