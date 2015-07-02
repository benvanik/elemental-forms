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

TBSectionHeader::TBSectionHeader() {
  SetSkinBg(TBIDC("TBSectionHeader"));
  SetGravity(Gravity::kLeft | Gravity::kRight);
  SetToggleMode(true);
}

bool TBSectionHeader::OnEvent(const TBWidgetEvent& ev) {
  if (ev.target == this && ev.type == EventType::kChanged &&
      GetParent()->GetParent()) {
    if (TBSection* section = TBSafeCast<TBSection>(GetParent()->GetParent())) {
      section->GetContainer()->SetValue(GetValue());

      // Try to scroll the container into view when expanded
      section->SetPendingScrollIntoView(GetValue() ? true : false);
    }
  }
  return TBButton::OnEvent(ev);
}

TBSection::TBSection() : m_pending_scroll(false) {
  SetGravity(Gravity::kLeft | Gravity::kRight);

  SetSkinBg(TBIDC("TBSection"), InvokeInfo::kNoCallbacks);
  m_layout.SetSkinBg(TBIDC("TBSection.layout"), InvokeInfo::kNoCallbacks);

  m_toggle_container.SetSkinBg(TBIDC("TBSection.container"));
  m_toggle_container.SetToggleAction(ToggleAction::kExpanded);
  m_toggle_container.SetGravity(Gravity::kAll);
  m_layout.SetAxis(Axis::kY);
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetLayoutSize(LayoutSize::kAvailable);

  AddChild(&m_layout);
  m_layout.AddChild(&m_header);
  m_layout.AddChild(&m_toggle_container);
}

TBSection::~TBSection() {
  m_layout.RemoveChild(&m_toggle_container);
  m_layout.RemoveChild(&m_header);
  RemoveChild(&m_layout);
}

void TBSection::SetValue(int value) {
  m_header.SetValue(value);
  m_toggle_container.SetValue(value);
}

void TBSection::OnProcessAfterChildren() {
  if (m_pending_scroll) {
    m_pending_scroll = false;
    ScrollIntoViewRecursive();
  }
}

PreferredSize TBSection::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = TBWidget::OnCalculatePreferredContentSize(constraints);
  // We should not grow larger than we are, when there's extra space available.
  ps.max_h = ps.pref_h;
  return ps;
}

TBToggleContainer::TBToggleContainer()
    : m_toggle(ToggleAction::kNothing), m_invert(false), m_value(0) {
  SetSkinBg(TBIDC("TBToggleContainer"), InvokeInfo::kNoCallbacks);
}

void TBToggleContainer::SetToggleAction(ToggleAction toggle) {
  if (toggle == m_toggle) return;

  if (m_toggle == ToggleAction::kExpanded) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  m_toggle = toggle;
  UpdateInternal();
}

void TBToggleContainer::SetInvert(bool invert) {
  if (invert == m_invert) return;
  m_invert = invert;
  UpdateInternal();
}

void TBToggleContainer::SetValue(int value) {
  if (value == m_value) return;
  m_value = value;
  UpdateInternal();
  InvalidateSkinStates();
}

void TBToggleContainer::UpdateInternal() {
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
