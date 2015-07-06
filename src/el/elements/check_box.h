/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_CHECK_BOX_H_
#define EL_ELEMENTS_CHECK_BOX_H_

#include "el/element.h"
#include "el/elements/parts/base_radio_check_box.h"

namespace el {
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
}  // namespace el

#endif  // EL_ELEMENTS_CHECK_BOX_H_
