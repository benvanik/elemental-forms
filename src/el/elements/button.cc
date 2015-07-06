/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/element_listener.h"
#include "el/elements/button.h"
#include "el/elements/parts/base_radio_check_box.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void Button::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Button, Value::Type::kNull, ElementZ::kBottom);
}

Button::Button() {
  set_focusable(true);
  set_click_by_key(true);
  set_background_skin(TBIDC("Button"), InvokeInfo::kNoCallbacks);
  AddChild(&m_layout);
  // Set the textfield gravity to all, even though it would display the same
  // with default gravity.
  // This will make the buttons layout expand if there is space available,
  // without forcing the parent
  // layout to grow to make the space available.
  m_textfield.set_gravity(Gravity::kAll);
  m_layout.AddChild(&m_textfield);
  m_layout.set_rect(padding_rect());
  m_layout.set_gravity(Gravity::kAll);
  m_layout.set_paint_overflow_fadeout(false);
}

Button::~Button() {
  m_layout.RemoveChild(&m_textfield);
  RemoveChild(&m_layout);
}

void Button::OnInflate(const parsing::InflateInfo& info) {
  set_toggle_mode(
      info.node->GetValueInt("toggle-mode", is_toggle_mode()) ? true : false);
  Element::OnInflate(info);
}

void Button::set_text(const char* text) {
  m_textfield.set_text(text);
  UpdateLabelVisibility();
}

int Button::value() { return has_state(Element::State::kPressed); }

void Button::set_value(int new_value) {
  if (new_value == value()) return;
  set_state(Element::State::kPressed, new_value ? true : false);

  if (can_toggle()) {
    // Invoke a changed event.
    Event ev(EventType::kChanged);
    InvokeEvent(ev);
  }

  if (new_value && group_id()) {
    parts::BaseRadioCheckBox::UpdateGroupElements(this);
  }
}

void Button::OnCaptureChanged(bool captured) {
  if (captured && m_auto_repeat_click) {
    PostMessageDelayed(TBIDC("auto_click"), nullptr,
                       kAutoClickFirstDelayMillis);
  } else if (!captured) {
    if (Message* msg = GetMessageById(TBIDC("auto_click"))) {
      DeleteMessage(msg);
    }
  }
}

void Button::OnSkinChanged() { m_layout.set_rect(padding_rect()); }

bool Button::OnEvent(const Event& ev) {
  if (can_toggle() && ev.type == EventType::kClick && ev.target == this) {
    WeakElementPointer this_element(this);

    // Toggle the value, if it's not a grouped element with value on.
    if (!(group_id() && value())) {
      set_value(!value());
    }

    if (!this_element.get()) {
      return true;  // We got removed so we actually handled this event.
    }

    // Intentionally don't return true for this event. We want it to continue
    // propagating.
  }
  return Element::OnEvent(ev);
}

void Button::OnMessageReceived(Message* msg) {
  if (msg->message_id() == TBIDC("auto_click")) {
    assert(captured_element == this);
    if (!cancel_click &&
        GetHitStatus(pointer_move_element_x, pointer_move_element_y) !=
            HitStatus::kNoHit) {
      Event ev(EventType::kClick, pointer_move_element_x,
               pointer_move_element_y, true);
      captured_element->InvokeEvent(ev);
    }
    if (kAutoClickRepeattDelayMillis) {
      PostMessageDelayed(TBIDC("auto_click"), nullptr,
                         kAutoClickRepeattDelayMillis);
    }
  }
}

HitStatus Button::GetHitStatus(int x, int y) {
  // Never hit any of the children to the button. We always want to the button
  // itself.
  return Element::GetHitStatus(x, y) != HitStatus::kNoHit
             ? HitStatus::kHitNoChildren
             : HitStatus::kNoHit;
}

void Button::UpdateLabelVisibility() {
  // Auto-collapse the textfield if the text is empty and there are other
  // elements added apart from the textfield. This removes the extra spacing
  // added between the textfield and the other element.
  bool collapse_textfield =
      m_textfield.empty() && m_layout.first_child() != m_layout.last_child();
  m_textfield.set_visibility(collapse_textfield ? Visibility::kGone
                                                : Visibility::kVisible);
}

void Button::ButtonLayoutBox::OnChildAdded(Element* child) {
  static_cast<Button*>(parent())->UpdateLabelVisibility();
}

void Button::ButtonLayoutBox::OnChildRemove(Element* child) {
  static_cast<Button*>(parent())->UpdateLabelVisibility();
}

}  // namespace elements
}  // namespace el
