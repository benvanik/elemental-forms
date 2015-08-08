/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_PARTS_BASE_RADIO_CHECK_BOX_H_
#define EL_ELEMENTS_PARTS_BASE_RADIO_CHECK_BOX_H_

#include "el/element.h"

namespace el {
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
  bool OnEvent(const Event& ev) override;

  // Makes sure all elements sharing the same group as new_leader are set to
  // value 0.
  static void UpdateGroupElements(Element* new_leader);

 protected:
  int m_value = 0;
};

}  // namespace parts
}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_PARTS_BASE_RADIO_CHECK_BOX_H_
