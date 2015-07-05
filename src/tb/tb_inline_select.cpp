/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>
#include <cstdlib>

#include "tb_inline_select.h"
#include "tb/parsing/element_factory.h"
#include "tb/util/math.h"

namespace tb {

// FIX: axis should affect the buttons arrow skin!
// FIX: unfocus should set the correct text!

void SelectInline::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(SelectInline, Value::Type::kInt, ElementZ::kTop);
}

SelectInline::SelectInline() {
  SetSkinBg(TBIDC("SelectInline"));
  AddChild(&m_layout);
  m_layout.AddChild(&m_buttons[0]);
  m_layout.AddChild(&m_text_box);
  m_layout.AddChild(&m_buttons[1]);
  m_layout.set_rect(GetPaddingRect());
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetSpacing(0);
  m_buttons[0].SetSkinBg(TBIDC("Button.flat"));
  m_buttons[1].SetSkinBg(TBIDC("Button.flat"));
  m_buttons[0].GetContentRoot()->AddChild(new SkinImage(TBIDC("arrow.left")));
  m_buttons[1].GetContentRoot()->AddChild(new SkinImage(TBIDC("arrow.right")));
  m_buttons[0].SetIsFocusable(false);
  m_buttons[1].SetIsFocusable(false);
  m_buttons[0].set_id(TBIDC("dec"));
  m_buttons[1].set_id(TBIDC("inc"));
  m_buttons[0].SetAutoRepeat(true);
  m_buttons[1].SetAutoRepeat(true);
  m_text_box.SetTextAlign(TextAlign::kCenter);
  m_text_box.SetEditType(EditType::kNumber);
  m_text_box.SetText("0");
}

SelectInline::~SelectInline() {
  m_layout.RemoveChild(&m_buttons[1]);
  m_layout.RemoveChild(&m_text_box);
  m_layout.RemoveChild(&m_buttons[0]);
  RemoveChild(&m_layout);
}

void SelectInline::OnInflate(const parsing::InflateInfo& info) {
  int min = info.node->GetValueInt("min", GetMinValue());
  int max = info.node->GetValueInt("max", GetMaxValue());
  SetLimits(min, max);
  Element::OnInflate(info);
}

void SelectInline::SetLimits(int min, int max) {
  assert(min <= max);
  m_min = min;
  m_max = max;
  SetValue(m_value);
}

void SelectInline::SetValueInternal(int value, bool update_text) {
  value = util::Clamp(value, m_min, m_max);
  if (value == m_value) return;
  m_value = value;

  if (update_text) {
    m_text_box.SetText(std::to_string(m_value));
  }

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);

  // Warning: Do nothing here since the event might have deleted us.
  //          If needed, check if we are alive using a safe pointer first.
}

void SelectInline::OnSkinChanged() { m_layout.set_rect(GetPaddingRect()); }

bool SelectInline::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kKeyDown) {
    if (ev.special_key == SpecialKey::kUp ||
        ev.special_key == SpecialKey::kDown) {
      int dv = ev.special_key == SpecialKey::kUp ? 1 : -1;
      SetValue(GetValue() + dv);
      return true;
    }
  } else if (ev.type == EventType::kClick && ev.target->id() == TBIDC("dec")) {
    SetValue(GetValue() - 1);
    return true;
  } else if (ev.type == EventType::kClick && ev.target->id() == TBIDC("inc")) {
    SetValue(GetValue() + 1);
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == &m_text_box) {
    auto text = m_text_box.GetText();
    SetValueInternal(atoi(text.c_str()), false);
  }
  return false;
}

}  // namespace tb
