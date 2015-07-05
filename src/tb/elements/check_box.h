/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_CHECK_BOX_H_
#define TB_ELEMENTS_CHECK_BOX_H_

#include "tb/element.h"
#include "tb/elements/parts/base_radio_check_box.h"

namespace tb {
namespace elements {

// A box toggling a check mark on click.
// For a labeled checkbox, use a LabelContainer containing a CheckBox.
class CheckBox : public parts::BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(CheckBox, BaseRadioCheckBox);
  static void RegisterInflater();

  CheckBox() {
    set_background_skin(TBIDC("CheckBox"), InvokeInfo::kNoCallbacks);
  }
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_CHECK_BOX_H_
