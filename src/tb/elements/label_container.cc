/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/label_container.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void LabelContainer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(LabelContainer, Value::Type::kString,
                               ElementZ::kBottom);
}

LabelContainer::LabelContainer() {
  AddChild(&m_layout);
  m_layout.AddChild(&m_textfield);
  m_layout.set_rect(GetPaddingRect());
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetLayoutDistributionPosition(LayoutDistributionPosition::kLeftTop);
}

LabelContainer::~LabelContainer() {
  m_layout.RemoveChild(&m_textfield);
  RemoveChild(&m_layout);
}

bool LabelContainer::OnEvent(const ElementEvent& ev) {
  // Get a element from the layout that isn't the textfield, or just bail out
  // if we only have the textfield.
  if (m_layout.GetFirstChild() == m_layout.GetLastChild()) {
    return false;
  }
  Element* click_target = m_layout.GetFirstChild() == &m_textfield
                              ? m_layout.GetLastChild()
                              : m_layout.GetFirstChild();
  // Invoke the event on it, as if it was invoked on the target itself.
  if (click_target && ev.target != click_target) {
    // Focus the target if we clicked the label.
    if (ev.type == EventType::kClick) {
      click_target->SetFocus(FocusReason::kPointer);
    }

    // Sync our pressed state with the click target. Special case for when we're
    // just about to lose it ourself (pointer is being released).
    bool pressed_state =
        any(ev.target->GetAutoState() & Element::State::kPressed);
    if (ev.type == EventType::kPointerUp || ev.type == EventType::kClick) {
      pressed_state = false;
    }

    click_target->SetState(Element::State::kPressed, pressed_state);

    ElementEvent target_ev(ev.type, ev.target_x - click_target->rect().x,
                           ev.target_y - click_target->rect().y, ev.touch,
                           ev.modifierkeys);
    return click_target->InvokeEvent(target_ev);
  }
  return false;
}

}  // namespace elements
}  // namespace tb
