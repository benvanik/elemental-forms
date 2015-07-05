/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/toggle_container.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void ToggleContainer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(ToggleContainer, Value::Type::kInt,
                               ElementZ::kTop);
}

ToggleContainer::ToggleContainer() {
  SetSkinBg(TBIDC("ToggleContainer"), InvokeInfo::kNoCallbacks);
}

void ToggleContainer::OnInflate(const parsing::InflateInfo& info) {
  if (const char* toggle = info.node->GetValueString("toggle", nullptr)) {
    SetToggleAction(from_string(toggle, GetToggleAction()));
  }
  SetInvert(info.node->GetValueInt("invert", GetInvert()) ? true : false);
  Element::OnInflate(info);
}

void ToggleContainer::SetToggleAction(ToggleAction toggle) {
  if (toggle == m_toggle) return;

  if (m_toggle == ToggleAction::kExpanded) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  m_toggle = toggle;
  UpdateInternal();
}

void ToggleContainer::SetInvert(bool invert) {
  if (invert == m_invert) return;
  m_invert = invert;
  UpdateInternal();
}

void ToggleContainer::SetValue(int value) {
  if (value == m_value) return;
  m_value = value;
  UpdateInternal();
  InvalidateSkinStates();
}

void ToggleContainer::UpdateInternal() {
  bool on = GetIsOn();
  switch (m_toggle) {
    case ToggleAction::kNothing:
      break;
    case ToggleAction::kEnabled:
      SetState(Element::State::kDisabled, !on);
      break;
    case ToggleAction::kOpacity:
      SetOpacity(on ? 1.f : 0);
      break;
    case ToggleAction::kExpanded:
      SetVisibilility(on ? Visibility::kVisible : Visibility::kGone);
      // Also disable when collapsed so tab focus skips the children.
      SetState(Element::State::kDisabled, !on);
      break;
  };
}

}  // namespace elements
}  // namespace tb
