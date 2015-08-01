/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <memory>

#include "el/elements/label_container.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void LabelContainer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(LabelContainer, Value::Type::kString,
                               ElementZ::kBottom);
}

LabelContainer::LabelContainer() {
  AddChild(&m_layout);
  m_layout.AddChild(&m_textfield);
  m_layout.set_rect(padding_rect());
  m_layout.set_gravity(Gravity::kAll);
  m_layout.set_layout_distribution_position(
      LayoutDistributionPosition::kLeftTop);
}

LabelContainer::~LabelContainer() {
  m_layout.RemoveChild(&m_textfield);
  RemoveChild(&m_layout);
}

bool LabelContainer::OnEvent(const Event& ev) {
  // Get a element from the layout that isn't the textfield, or just bail out
  // if we only have the textfield.
  if (m_layout.first_child() == m_layout.last_child()) {
    return Element::OnEvent(ev);
  }
  Element* click_target = m_layout.first_child() == &m_textfield
                              ? m_layout.last_child()
                              : m_layout.first_child();
  // Invoke the event on it, as if it was invoked on the target itself.
  if (click_target && ev.target != click_target) {
    // Focus the target if we clicked the label.
    if (ev.type == EventType::kClick) {
      click_target->set_focus(FocusReason::kPointer);
    }

    // Sync our pressed state with the click target. Special case for when we're
    // just about to lose it ourself (pointer is being released).
    bool pressed_state =
        any(ev.target->computed_state() & Element::State::kPressed);
    if (ev.type == EventType::kPointerUp || ev.type == EventType::kClick) {
      pressed_state = false;
    }

    click_target->set_state(Element::State::kPressed, pressed_state);

    Event target_ev(ev.type, ev.target_x - click_target->rect().x,
                    ev.target_y - click_target->rect().y, ev.touch,
                    ev.modifierkeys);
    if (click_target->InvokeEvent(target_ev)) {
      return true;
    }
  }
  return Element::OnEvent(ev);
}

}  // namespace elements
}  // namespace el
