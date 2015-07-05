/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_INLINE_SELECT_H
#define TB_INLINE_SELECT_H

#include "tb_layout.h"
#include "tb_select_item.h"
#include "tb_text_box.h"

#include "tb/elements/button.h"

namespace tb {
namespace elements {

// InlineSelect is a select element with no popups. Instead it has two arrow
// buttons that cycle between the choices. By default it is a number element.
// FIX: Should also be possible to set a list of strings that will be shown
// instead of numbers.
class SelectInline : public Element {
 public:
  TBOBJECT_SUBCLASS(SelectInline, Element);
  static void RegisterInflater();

  SelectInline();
  ~SelectInline() override;

  // Sets along which axis the content should layout.
  void SetAxis(Axis axis) override { m_layout.SetAxis(axis); }
  Axis GetAxis() const override { return m_layout.GetAxis(); }

  void SetLimits(int min, int max);
  int GetMinValue() const { return m_min; }
  int GetMaxValue() const { return m_max; }

  void SetValue(int value) override { SetValueInternal(value, true); }
  int GetValue() override { return m_value; }

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnSkinChanged() override;
  bool OnEvent(const ElementEvent& ev) override;

 protected:
  void SetValueInternal(int value, bool update_text);

  Button m_buttons[2];
  Layout m_layout;
  TextBox m_text_box;
  int m_value = 0;
  int m_min = 0;
  int m_max = 100;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_INLINE_SELECT_H
