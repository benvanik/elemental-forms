/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_inline_select.h"

#include <cassert>
#include <cstdlib>

namespace tb {

// FIX: axis should affect the buttons arrow skin!
// FIX: unfocus should set the correct text!

SelectInline::SelectInline() {
  SetSkinBg(TBIDC("SelectInline"));
  AddChild(&m_layout);
  m_layout.AddChild(&m_buttons[0]);
  m_layout.AddChild(&m_editfield);
  m_layout.AddChild(&m_buttons[1]);
  m_layout.SetRect(GetPaddingRect());
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetSpacing(0);
  m_buttons[0].SetSkinBg(TBIDC("Button.flat"));
  m_buttons[1].SetSkinBg(TBIDC("Button.flat"));
  m_buttons[0].GetContentRoot()->AddChild(new SkinImage(TBIDC("arrow.left")));
  m_buttons[1].GetContentRoot()->AddChild(new SkinImage(TBIDC("arrow.right")));
  m_buttons[0].SetIsFocusable(false);
  m_buttons[1].SetIsFocusable(false);
  m_buttons[0].SetID(TBIDC("dec"));
  m_buttons[1].SetID(TBIDC("inc"));
  m_buttons[0].SetAutoRepeat(true);
  m_buttons[1].SetAutoRepeat(true);
  m_editfield.SetTextAlign(TextAlign::kCenter);
  m_editfield.SetEditType(EditType::kNumber);
  m_editfield.SetText("0");
}

SelectInline::~SelectInline() {
  m_layout.RemoveChild(&m_buttons[1]);
  m_layout.RemoveChild(&m_editfield);
  m_layout.RemoveChild(&m_buttons[0]);
  RemoveChild(&m_layout);
}

void SelectInline::SetLimits(int min, int max) {
  assert(min <= max);
  m_min = min;
  m_max = max;
  SetValue(m_value);
}

void SelectInline::SetValueInternal(int value, bool update_text) {
  value = Clamp(value, m_min, m_max);
  if (value == m_value) return;
  m_value = value;

  if (update_text) {
    m_editfield.SetText(std::to_string(m_value));
  }

  TBWidgetEvent ev(EventType::kChanged);
  InvokeEvent(ev);

  // Warning: Do nothing here since the event might have deleted us.
  //          If needed, check if we are alive using a safe pointer first.
}

void SelectInline::OnSkinChanged() { m_layout.SetRect(GetPaddingRect()); }

bool SelectInline::OnEvent(const TBWidgetEvent& ev) {
  if (ev.type == EventType::kKeyDown) {
    if (ev.special_key == SpecialKey::kUp ||
        ev.special_key == SpecialKey::kDown) {
      int dv = ev.special_key == SpecialKey::kUp ? 1 : -1;
      SetValue(GetValue() + dv);
      return true;
    }
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("dec")) {
    SetValue(GetValue() - 1);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("inc")) {
    SetValue(GetValue() + 1);
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == &m_editfield) {
    auto text = m_editfield.GetText();
    SetValueInternal(atoi(text.c_str()), false);
  }
  return false;
}

}  // namespace tb
