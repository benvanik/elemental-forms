/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Seger�s and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_CHECK_BOX_H_
#define TB_ELEMENTS_CHECK_BOX_H_

#include "tb/element.h"
#include "tb/parts/toggle_button_base.h"

namespace tb {
namespace elements {

// A box toggling a check mark on click.
// For a labeled checkbox, use a LabelContainer containing a CheckBox.
class CheckBox : public parts::BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(CheckBox, BaseRadioCheckBox);
  static void RegisterInflater();

  CheckBox() { SetSkinBg(TBIDC("CheckBox"), InvokeInfo::kNoCallbacks); }
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_CHECK_BOX_H_