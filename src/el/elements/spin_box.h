/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_SPIN_BOX_H_
#define EL_ELEMENTS_SPIN_BOX_H_

#include "el/element.h"
#include "el/elements/button.h"
#include "el/elements/layout_box.h"
#include "el/elements/text_box.h"
#include "el/list_item.h"

namespace el {
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
}  // namespace el

#endif  // EL_ELEMENTS_SPIN_BOX_H_
