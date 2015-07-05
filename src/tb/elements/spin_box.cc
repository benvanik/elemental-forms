/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/icon_box.h"
#include "tb/elements/spin_box.h"
#include "tb/parsing/element_inflater.h"
#include "tb/util/math.h"

namespace tb {
namespace elements {

// FIX: axis should affect the buttons arrow skin!
// FIX: unfocus should set the correct text!

void SpinBox::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(SpinBox, Value::Type::kInt, ElementZ::kTop);
}

SpinBox::SpinBox() {
  set_background_skin(TBIDC("SpinBox"));
  AddChild(&m_layout);
  m_layout.AddChild(&m_buttons[0]);
  m_layout.AddChild(&m_text_box);
  m_layout.AddChild(&m_buttons[1]);
  m_layout.set_rect(padding_rect());
  m_layout.set_gravity(Gravity::kAll);
  m_layout.set_spacing(0);
  m_buttons[0].set_background_skin(TBIDC("Button.flat"));
  m_buttons[1].set_background_skin(TBIDC("Button.flat"));
  m_buttons[0].content_root()->AddChild(new IconBox(TBIDC("arrow.left")));
  m_buttons[1].content_root()->AddChild(new IconBox(TBIDC("arrow.right")));
  m_buttons[0].set_focusable(false);
  m_buttons[1].set_focusable(false);
  m_buttons[0].set_id(TBIDC("dec"));
  m_buttons[1].set_id(TBIDC("inc"));
  m_buttons[0].set_auto_repeat(true);
  m_buttons[1].set_auto_repeat(true);
  m_text_box.set_text_align(TextAlign::kCenter);
  m_text_box.set_edit_type(EditType::kNumber);
  m_text_box.set_text("0");
}

SpinBox::~SpinBox() {
  m_layout.RemoveChild(&m_buttons[1]);
  m_layout.RemoveChild(&m_text_box);
  m_layout.RemoveChild(&m_buttons[0]);
  RemoveChild(&m_layout);
}

void SpinBox::OnInflate(const parsing::InflateInfo& info) {
  int min = info.node->GetValueInt("min", min_value());
  int max = info.node->GetValueInt("max", max_value());
  set_limits(min, max);
  Element::OnInflate(info);
}

void SpinBox::set_limits(int min, int max) {
  assert(min <= max);
  m_min = min;
  m_max = max;
  set_value(m_value);
}

void SpinBox::SetValueInternal(int value, bool update_text) {
  value = util::Clamp(value, m_min, m_max);
  if (value == m_value) return;
  m_value = value;

  if (update_text) {
    m_text_box.set_text(std::to_string(m_value));
  }

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);

  // Warning: Do nothing here since the event might have deleted us.
  //          If needed, check if we are alive using a safe pointer first.
}

void SpinBox::OnSkinChanged() { m_layout.set_rect(padding_rect()); }

bool SpinBox::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kKeyDown) {
    if (ev.special_key == SpecialKey::kUp ||
        ev.special_key == SpecialKey::kDown) {
      int dv = ev.special_key == SpecialKey::kUp ? 1 : -1;
      set_value(value() + dv);
      return true;
    }
  } else if (ev.type == EventType::kClick && ev.target->id() == TBIDC("dec")) {
    set_value(value() - 1);
    return true;
  } else if (ev.type == EventType::kClick && ev.target->id() == TBIDC("inc")) {
    set_value(value() + 1);
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == &m_text_box) {
    auto text = m_text_box.text();
    SetValueInternal(atoi(text.c_str()), false);
  }
  return false;
}

}  // namespace elements
}  // namespace tb
