/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_ICON_BOX_H_
#define TB_ELEMENTS_ICON_BOX_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// A element showing a skin element, constrained in size to its skin.
// If you need to load and show images dynamically (i.e. not always loaded as
// the skin), you can use ImageBox.
class IconBox : public Element {
 public:
  TBOBJECT_SUBCLASS(IconBox, Element);
  static void RegisterInflater();

  IconBox() = default;
  IconBox(const TBID& skin_bg) { set_background_skin(skin_bg); }

  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_ICON_BOX_H_
