/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_RADIO_BUTTON_H_
#define EL_ELEMENTS_RADIO_BUTTON_H_

#include "el/element.h"
#include "el/elements/parts/base_radio_check_box.h"

namespace el {
namespace elements {

// A button which unselects other radiobuttons of the same group number when
// clicked.
// For a labeled radio button, use a LabelContainer containing a RadioButton.
class RadioButton : public parts::BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(RadioButton, BaseRadioCheckBox);
  static void RegisterInflater();

  RadioButton() {
    set_background_skin(TBIDC("RadioButton"), InvokeInfo::kNoCallbacks);
  }
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_RADIO_BUTTON_H_
