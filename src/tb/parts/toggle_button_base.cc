/**
******************************************************************************
* xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
******************************************************************************
* Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
* See turbo_badger.h and LICENSE in the root for more information.           *
******************************************************************************
*/

#include "tb/parsing/element_inflater.h"
#include "tb/parts/toggle_button_base.h"

namespace tb {
namespace parts {

BaseRadioCheckBox::BaseRadioCheckBox() {
  SetIsFocusable(true);
  SetClickByKey(true);
}

// static
void BaseRadioCheckBox::UpdateGroupElements(Element* new_leader) {
  assert(new_leader->GetValue() && new_leader->GetGroupID());

  // Find the group root element.
  Element* group = new_leader;
  while (group && !group->GetIsGroupRoot() && group->GetParent()) {
    group = group->GetParent();
  }

  for (Element* child = group; child; child = child->GetNextDeep(group)) {
    if (child != new_leader &&
        child->GetGroupID() == new_leader->GetGroupID()) {
      child->SetValue(0);
    }
  }
}

void BaseRadioCheckBox::SetValue(int value) {
  if (m_value == value) return;
  m_value = value;

  SetState(Element::State::kSelected, value ? true : false);

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);

  if (value && GetGroupID()) UpdateGroupElements(this);
}

PreferredSize BaseRadioCheckBox::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredSize(constraints);
  ps.min_w = ps.max_w = ps.pref_w;
  ps.min_h = ps.max_h = ps.pref_h;
  return ps;
}

bool BaseRadioCheckBox::OnEvent(const ElementEvent& ev) {
  if (ev.target == this && ev.type == EventType::kClick) {
    // Toggle the value, if it's not a grouped element with value on.
    if (!(GetGroupID() && GetValue())) {
      SetValue(!GetValue());
    }
  }
  return false;
}

}  // namespace parts
}  // namespace tb
