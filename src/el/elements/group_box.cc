/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/group_box.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

GroupBox::Header::Header() {
  set_background_skin(TBIDC("GroupBoxHeader"));
  set_gravity(Gravity::kLeft | Gravity::kRight);
  set_toggle_mode(true);
}

bool GroupBox::Header::OnEvent(const Event& ev) {
  if (ev.target == this && ev.type == EventType::kChanged &&
      parent()->parent()) {
    if (GroupBox* section = util::SafeCast<GroupBox>(parent()->parent())) {
      section->container()->set_value(value());

      // Try to scroll the container into view when expanded.
      section->set_pending_scroll_into_view(value() ? true : false);
    }
  }
  return Button::OnEvent(ev);
}

GroupBox::GroupBox() {
  set_gravity(Gravity::kLeft | Gravity::kRight);

  set_background_skin(TBIDC("GroupBox"), InvokeInfo::kNoCallbacks);
  m_layout.set_background_skin(TBIDC("GroupBox.layout"),
                               InvokeInfo::kNoCallbacks);

  m_toggle_container.set_background_skin(TBIDC("GroupBox.container"));
  m_toggle_container.set_toggle_action(ToggleAction::kExpanded);
  m_toggle_container.set_gravity(Gravity::kAll);
  m_layout.set_axis(Axis::kY);
  m_layout.set_gravity(Gravity::kAll);
  m_layout.set_layout_size(LayoutSize::kAvailable);

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
  EL_REGISTER_ELEMENT_INFLATER(GroupBox, Value::Type::kInt, ElementZ::kTop);
}

void GroupBox::set_value(int value) {
  m_header.set_value(value);
  m_toggle_container.set_value(value);
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
}  // namespace el
