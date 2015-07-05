/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/group_box.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

GroupBox::Header::Header() {
  SetSkinBg(TBIDC("GroupBoxHeader"));
  SetGravity(Gravity::kLeft | Gravity::kRight);
  SetToggleMode(true);
}

bool GroupBox::Header::OnEvent(const ElementEvent& ev) {
  if (ev.target == this && ev.type == EventType::kChanged &&
      GetParent()->GetParent()) {
    if (GroupBox* section =
            util::SafeCast<GroupBox>(GetParent()->GetParent())) {
      section->GetContainer()->SetValue(GetValue());

      // Try to scroll the container into view when expanded.
      section->SetPendingScrollIntoView(GetValue() ? true : false);
    }
  }
  return Button::OnEvent(ev);
}

GroupBox::GroupBox() {
  SetGravity(Gravity::kLeft | Gravity::kRight);

  SetSkinBg(TBIDC("GroupBox"), InvokeInfo::kNoCallbacks);
  m_layout.SetSkinBg(TBIDC("GroupBox.layout"), InvokeInfo::kNoCallbacks);

  m_toggle_container.SetSkinBg(TBIDC("GroupBox.container"));
  m_toggle_container.SetToggleAction(ToggleAction::kExpanded);
  m_toggle_container.SetGravity(Gravity::kAll);
  m_layout.SetAxis(Axis::kY);
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetLayoutSize(LayoutSize::kAvailable);

  AddChild(&m_layout);
  m_layout.AddChild(&m_header);
  m_layout.AddChild(&m_toggle_container);
}

GroupBox::~GroupBox() {
  m_layout.RemoveChild(&m_toggle_container);
  m_layout.RemoveChild(&m_header);
  RemoveChild(&m_layout);
}

void GroupBox::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(GroupBox, Value::Type::kInt, ElementZ::kTop);
}

void GroupBox::SetValue(int value) {
  m_header.SetValue(value);
  m_toggle_container.SetValue(value);
}

void GroupBox::OnProcessAfterChildren() {
  if (m_pending_scroll) {
    m_pending_scroll = false;
    ScrollIntoViewRecursive();
  }
}

PreferredSize GroupBox::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredContentSize(constraints);
  // We should not grow larger than we are, when there's extra space available.
  ps.max_h = ps.pref_h;
  return ps;
}

}  // namespace elements
}  // namespace tb
