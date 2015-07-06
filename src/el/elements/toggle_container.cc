/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/toggle_container.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void ToggleContainer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(ToggleContainer, Value::Type::kInt,
                               ElementZ::kTop);
}

ToggleContainer::ToggleContainer() {
  set_background_skin(TBIDC("ToggleContainer"), InvokeInfo::kNoCallbacks);
}

void ToggleContainer::OnInflate(const parsing::InflateInfo& info) {
  if (const char* toggle = info.node->GetValueString("toggle", nullptr)) {
    set_toggle_action(from_string(toggle, toggle_action()));
  }
  set_inverted(info.node->GetValueInt("invert", is_inverted()) ? true : false);
  Element::OnInflate(info);
}

void ToggleContainer::set_toggle_action(ToggleAction toggle) {
  if (toggle == m_toggle) return;

  if (m_toggle == ToggleAction::kExpanded) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  m_toggle = toggle;
  UpdateInternal();
}

void ToggleContainer::set_inverted(bool invert) {
  if (invert == m_invert) return;
  m_invert = invert;
  UpdateInternal();
}

void ToggleContainer::set_value(int value) {
  if (value == m_value) return;
  m_value = value;
  UpdateInternal();
  InvalidateSkinStates();
}

void ToggleContainer::UpdateInternal() {
  bool on = is_toggled();
  switch (m_toggle) {
    case ToggleAction::kNothing:
      break;
    case ToggleAction::kEnabled:
      set_state(Element::State::kDisabled, !on);
      break;
    case ToggleAction::kOpacity:
      set_opacity(on ? 1.f : 0);
      break;
    case ToggleAction::kExpanded:
      set_visibility(on ? Visibility::kVisible : Visibility::kGone);
      // Also disable when collapsed so tab focus skips the children.
      set_state(Element::State::kDisabled, !on);
      break;
  };
}

}  // namespace elements
}  // namespace el
