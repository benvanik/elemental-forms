/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_SPIN_BOX_H_
#define TB_ELEMENTS_SPIN_BOX_H_

#include "tb/element.h"
#include "tb/elements/button.h"
#include "tb/elements/layout_box.h"
#include "tb/elements/text_box.h"
#include "tb/list_item.h"

namespace tb {
namespace elements {

// TODO(benvanik): also support a list of strings that will be shown instead of
// numbers.

// A select element with no popups. Instead it has two arrow buttons that cycle
// between the choices. By default it is a number element.
class SpinBox : public Element {
 public:
  TBOBJECT_SUBCLASS(SpinBox, Element);
  static void RegisterInflater();

  SpinBox();
  ~SpinBox() override;

  Axis axis() const override { return m_layout.axis(); }
  // Sets along which axis the content should layout.
  void set_axis(Axis axis) override { m_layout.set_axis(axis); }

  int min_value() const { return m_min; }
  int max_value() const { return m_max; }
  void set_limits(int min, int max);

  int value() override { return m_value; }
  void set_value(int value) override { SetValueInternal(value, true); }

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnSkinChanged() override;
  bool OnEvent(const Event& ev) override;

 protected:
  void SetValueInternal(int value, bool update_text);

  Button m_buttons[2];
  LayoutBox m_layout;
  TextBox m_text_box;
  int m_value = 0;
  int m_min = 0;
  int m_max = 100;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_SPIN_BOX_H_
