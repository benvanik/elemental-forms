/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/element_listener.h"
#include "tb/elements/button.h"
#include "tb/parsing/element_inflater.h"
#include "tb/parts/toggle_button_base.h"

namespace tb {
namespace elements {

void Button::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Button, Value::Type::kNull, ElementZ::kBottom);
}

Button::Button() {
  SetIsFocusable(true);
  SetClickByKey(true);
  SetSkinBg(TBIDC("Button"), InvokeInfo::kNoCallbacks);
  AddChild(&m_layout);
  // Set the textfield gravity to all, even though it would display the same
  // with default gravity.
  // This will make the buttons layout expand if there is space available,
  // without forcing the parent
  // layout to grow to make the space available.
  m_textfield.SetGravity(Gravity::kAll);
  m_layout.AddChild(&m_textfield);
  m_layout.set_rect(GetPaddingRect());
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetPaintOverflowFadeout(false);
}

Button::~Button() {
  m_layout.RemoveChild(&m_textfield);
  RemoveChild(&m_layout);
}

void Button::OnInflate(const parsing::InflateInfo& info) {
  SetToggleMode(info.node->GetValueInt("toggle-mode", GetToggleMode()) ? true
                                                                       : false);
  Element::OnInflate(info);
}

void Button::SetText(const char* text) {
  m_textfield.SetText(text);
  UpdateLabelVisibility();
}

void Button::SetValue(int value) {
  if (value == GetValue()) return;
  SetState(Element::State::kPressed, value ? true : false);

  if (CanToggle()) {
    // Invoke a changed event.
    ElementEvent ev(EventType::kChanged);
    InvokeEvent(ev);
  }

  if (value && GetGroupID()) {
    parts::BaseRadioCheckBox::UpdateGroupElements(this);
  }
}

int Button::GetValue() { return GetState(Element::State::kPressed); }

void Button::OnCaptureChanged(bool captured) {
  if (captured && m_auto_repeat_click) {
    PostMessageDelayed(TBIDC("auto_click"), nullptr,
                       kAutoClickFirstDelayMillis);
  } else if (!captured) {
    if (Message* msg = GetMessageByID(TBIDC("auto_click"))) {
      DeleteMessage(msg);
    }
  }
}

void Button::OnSkinChanged() { m_layout.set_rect(GetPaddingRect()); }

bool Button::OnEvent(const ElementEvent& ev) {
  if (CanToggle() && ev.type == EventType::kClick && ev.target == this) {
    WeakElementPointer this_element(this);

    // Toggle the value, if it's not a grouped element with value on.
    if (!(GetGroupID() && GetValue())) {
      SetValue(!GetValue());
    }

    if (!this_element.Get()) {
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
      ElementEvent ev(EventType::kClick, pointer_move_element_x,
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
  bool collapse_textfield = m_textfield.empty() &&
                            m_layout.GetFirstChild() != m_layout.GetLastChild();
  m_textfield.SetVisibilility(collapse_textfield ? Visibility::kGone
                                                 : Visibility::kVisible);
}

void Button::ButtonLayout::OnChildAdded(Element* child) {
  static_cast<Button*>(GetParent())->UpdateLabelVisibility();
}

void Button::ButtonLayout::OnChildRemove(Element* child) {
  static_cast<Button*>(GetParent())->UpdateLabelVisibility();
}

}  // namespace elements
}  // namespace tb
