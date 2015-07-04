/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_toggle_container.h"

#include "tb_node_tree.h"
#include "tb_widgets_reader.h"

namespace tb {

SectionHeader::SectionHeader() {
  SetSkinBg(TBIDC("SectionHeader"));
  SetGravity(Gravity::kLeft | Gravity::kRight);
  SetToggleMode(true);
}

bool SectionHeader::OnEvent(const ElementEvent& ev) {
  if (ev.target == this && ev.type == EventType::kChanged &&
      GetParent()->GetParent()) {
    if (Section* section = util::SafeCast<Section>(GetParent()->GetParent())) {
      section->GetContainer()->SetValue(GetValue());

      // Try to scroll the container into view when expanded.
      section->SetPendingScrollIntoView(GetValue() ? true : false);
    }
  }
  return Button::OnEvent(ev);
}

Section::Section() {
  SetGravity(Gravity::kLeft | Gravity::kRight);

  SetSkinBg(TBIDC("Section"), InvokeInfo::kNoCallbacks);
  m_layout.SetSkinBg(TBIDC("Section.layout"), InvokeInfo::kNoCallbacks);

  m_toggle_container.SetSkinBg(TBIDC("Section.container"));
  m_toggle_container.SetToggleAction(ToggleAction::kExpanded);
  m_toggle_container.SetGravity(Gravity::kAll);
  m_layout.SetAxis(Axis::kY);
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetLayoutSize(LayoutSize::kAvailable);

  AddChild(&m_layout);
  m_layout.AddChild(&m_header);
  m_layout.AddChild(&m_toggle_container);
}

Section::~Section() {
  m_layout.RemoveChild(&m_toggle_container);
  m_layout.RemoveChild(&m_header);
  RemoveChild(&m_layout);
}

void Section::SetValue(int value) {
  m_header.SetValue(value);
  m_toggle_container.SetValue(value);
}

void Section::OnProcessAfterChildren() {
  if (m_pending_scroll) {
    m_pending_scroll = false;
    ScrollIntoViewRecursive();
  }
}

PreferredSize Section::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredContentSize(constraints);
  // We should not grow larger than we are, when there's extra space available.
  ps.max_h = ps.pref_h;
  return ps;
}

ToggleContainer::ToggleContainer() {
  SetSkinBg(TBIDC("ToggleContainer"), InvokeInfo::kNoCallbacks);
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
      SetState(SkinState::kDisabled, !on);
      break;
    case ToggleAction::kOpacity:
      SetOpacity(on ? 1.f : 0);
      break;
    case ToggleAction::kExpanded:
      SetVisibilility(on ? Visibility::kVisible : Visibility::kGone);
      // Also disable when collapsed so tab focus skips the children.
      SetState(SkinState::kDisabled, !on);
      break;
  };
}

}  // namespace tb
