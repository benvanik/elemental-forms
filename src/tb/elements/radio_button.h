/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_RADIO_BUTTON_H_
#define TB_ELEMENTS_RADIO_BUTTON_H_

#include "tb/element.h"
#include "tb/parts/toggle_button_base.h"

namespace tb {
namespace elements {

// A button which unselects other radiobuttons of the same group number when
// clicked.
// For a labeled radio button, use a LabelContainer containing a RadioButton.
class RadioButton : public parts::BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(RadioButton, BaseRadioCheckBox);
  static void RegisterInflater();

  RadioButton() { SetSkinBg(TBIDC("RadioButton"), InvokeInfo::kNoCallbacks); }
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_RADIO_BUTTON_H_
