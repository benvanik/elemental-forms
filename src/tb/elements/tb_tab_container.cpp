/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "tb_tab_container.h"

#include "tb/elements/button.h"
#include "tb/parsing/element_inflater.h"

namespace tb {
namespace elements {

void TabLayout::OnChildAdded(Element* child) {
  if (auto button = util::SafeCast<Button>(child)) {
    button->SetSqueezable(true);
    button->SetSkinBg(TBIDC("TabContainer.tab"));
    button->set_id(TBIDC("tab"));
  }
}

PreferredSize TabLayout::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Layout::OnCalculatePreferredContentSize(constraints);
  // Make sure the number of tabs doesn't grow parents.
  // It is only the content that should do that. The tabs
  // will scroll anyway.
  if (GetAxis() == Axis::kX) {
    ps.min_w = std::min(ps.min_w, 1);
  } else {
    ps.min_h = std::min(ps.min_h, 1);
  }
  return ps;
}

void TabContainer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(TabContainer, Value::Type::kNull,
                               ElementZ::kTop);
}

TabContainer::TabContainer() {
  AddChild(&m_root_layout);
  // Put the tab layout on top of the content in Z order so their skin can make
  // a seamless overlap over the border. Control which side they are layouted
  // to by calling SetLayoutOrder.
  m_root_layout.AddChild(&m_content_root);
  m_root_layout.AddChild(&m_tab_layout);
  m_root_layout.SetAxis(Axis::kY);
  m_root_layout.SetGravity(Gravity::kAll);
  m_root_layout.SetLayoutDistribution(LayoutDistribution::kAvailable);
  m_root_layout.SetLayoutOrder(LayoutOrder::kTopToBottom);
  m_root_layout.SetSkinBg(TBIDC("TabContainer.rootlayout"));
  m_tab_layout.SetLayoutDistributionPosition(
      LayoutDistributionPosition::kCenter);
  m_tab_layout.SetSkinBg(TBIDC("TabContainer.tablayout_x"));
  m_tab_layout.SetLayoutPosition(LayoutPosition::kRightBottom);
  m_content_root.SetGravity(Gravity::kAll);
  m_content_root.SetSkinBg(TBIDC("TabContainer.container"));
}

TabContainer::~TabContainer() {
  m_root_layout.RemoveChild(&m_content_root);
  m_root_layout.RemoveChild(&m_tab_layout);
  RemoveChild(&m_root_layout);
}

void TabContainer::OnInflate(const parsing::InflateInfo& info) {
  Element::OnInflate(info);

  if (const char* align = info.node->GetValueString("align", nullptr)) {
    SetAlignment(tb::from_string(align, GetAlignment()));
  }
  // Allow additional attributes to be specified for the "tabs", "content" and
  // "root" layouts by calling OnInflate.
  if (auto tabs = info.node->GetNode("tabs")) {
    // Inflate the tabs elements into the tab layout.
    Layout* tab_layout = GetTabLayout();
    info.reader->LoadNodeTree(tab_layout, tabs);

    parsing::InflateInfo inflate_info(info.reader, tab_layout->GetContentRoot(),
                                      tabs, Value::Type::kNull);
    tab_layout->OnInflate(inflate_info);
  }
  if (auto tabs = info.node->GetNode("content")) {
    parsing::InflateInfo inflate_info(info.reader, GetContentRoot(), tabs,
                                      Value::Type::kNull);
    GetContentRoot()->OnInflate(inflate_info);
  }
  if (auto tabs = info.node->GetNode("root")) {
    parsing::InflateInfo inflate_info(info.reader, &m_root_layout, tabs,
                                      Value::Type::kNull);
    m_root_layout.OnInflate(inflate_info);
  }
}

void TabContainer::SetAxis(Axis axis) {
  m_root_layout.SetAxis(axis);
  m_tab_layout.SetAxis(axis == Axis::kX ? Axis::kY : Axis::kX);
  m_tab_layout.SetSkinBg(axis == Axis::kX ? TBIDC("TabContainer.tablayout_y")
                                          : TBIDC("TabContainer.tablayout_x"));
}

void TabContainer::SetValue(int index) {
  if (index == m_current_page) return;
  m_current_page = index;

  // Update the pages visibility and tabs pressed value.
  index = 0;
  Element* page = m_content_root.GetFirstChild();
  Element* tab = m_tab_layout.GetFirstChild();
  for (; page && tab; page = page->GetNext(), tab = tab->GetNext(), index++) {
    bool active = index == m_current_page;
    page->SetVisibilility(active ? Visibility::kVisible
                                 : Visibility::kInvisible);
    tab->SetValue(active ? 1 : 0);
  }
}

int TabContainer::GetNumPages() {
  int count = 0;
  for (Element* tab = m_tab_layout.GetFirstChild(); tab; tab = tab->GetNext()) {
    count++;
  }
  return count;
}

Element* TabContainer::GetCurrentPageElement() const {
  return m_content_root.GetChildFromIndex(m_current_page);
}

void TabContainer::SetAlignment(Align align) {
  bool horizontal = (align == Align::kTop || align == Align::kBottom);
  bool reverse = (align == Align::kTop || align == Align::kLeft);
  SetAxis(horizontal ? Axis::kY : Axis::kX);
  m_root_layout.SetLayoutOrder(reverse ? LayoutOrder::kTopToBottom
                                       : LayoutOrder::kBottomToTop);
  m_tab_layout.SetLayoutPosition(reverse ? LayoutPosition::kRightBottom
                                         : LayoutPosition::kLeftTop);
  m_align = align;
}

bool TabContainer::OnEvent(const ElementEvent& ev) {
  if ((ev.type == EventType::kClick || ev.type == EventType::kPointerDown) &&
      ev.target->id() == TBIDC("tab") &&
      ev.target->GetParent() == &m_tab_layout) {
    int clicked_index = m_tab_layout.GetIndexFromChild(ev.target);
    SetValue(clicked_index);
    return true;
  }
  return false;
}

void TabContainer::OnProcess() {
  if (m_need_page_update) {
    m_need_page_update = false;
    // Force update value.
    int current_page = m_current_page;
    m_current_page = -1;
    SetValue(current_page);
  }
}

}  // namespace elements
}  // namespace tb
