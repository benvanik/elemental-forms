/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_INLINE_SELECT_H
#define TB_INLINE_SELECT_H

#include "tb_editfield.h"
#include "tb_select_item.h"
#include "tb_widgets_listener.h"

namespace tb {

// InlineSelect is a select widget with no popups. Instead it has two arrow
// buttons that cycle between the choices. By default it is a number widget.
// FIX: Should also be possible to set a list of strings that will be shown
// instead of numbers.
class SelectInline : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(SelectInline, TBWidget);

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

  void OnInflate(const INFLATE_INFO& info) override;
  void OnSkinChanged() override;
  bool OnEvent(const TBWidgetEvent& ev) override;

 protected:
  void SetValueInternal(int value, bool update_text);

  Button m_buttons[2];
  TBLayout m_layout;
  TBEditField m_editfield;
  int m_value = 0;
  int m_min = 0;
  int m_max = 100;
};

}  // namespace tb

#endif  // TB_INLINE_SELECT_H
