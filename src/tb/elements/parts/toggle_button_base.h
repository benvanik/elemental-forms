/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_PARTS_TOGGLE_BUTTON_BASE_H_
#define TB_ELEMENTS_PARTS_TOGGLE_BUTTON_BASE_H_

#include "tb/element.h"

namespace tb {
namespace elements {
namespace parts {

// Shared functionality for CheckBox and RadioButton.
class BaseRadioCheckBox : public Element {
 public:
  TBOBJECT_SUBCLASS(BaseRadioCheckBox, Element);

  BaseRadioCheckBox();

  void set_value(int value) override;
  int value() override { return m_value; }

  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;
  bool OnEvent(const ElementEvent& ev) override;

  // Makes sure all elements sharing the same group as new_leader are set to
  // value 0.
  static void UpdateGroupElements(Element* new_leader);

 protected:
  int m_value = 0;
};

}  // namespace parts
}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_PARTS_TOGGLE_BUTTON_BASE_H_
