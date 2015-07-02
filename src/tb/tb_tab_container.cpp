/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_tab_container.h"

#include <algorithm>
#include <cassert>

namespace tb {

void TBTabLayout::OnChildAdded(TBWidget* child) {
  if (TBButton* button = TBSafeCast<TBButton>(child)) {
    button->SetSqueezable(true);
    button->SetSkinBg(TBIDC("TBTabContainer.tab"));
    button->SetID(TBIDC("tab"));
  }
}

PreferredSize TBTabLayout::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = TBLayout::OnCalculatePreferredContentSize(constraints);
  // Make sure the number of tabs doesn't grow parents.
  // It is only the content that should do that. The tabs
  // will scroll anyway.
  if (GetAxis() == Axis::kX)
    ps.min_w = std::min(ps.min_w, 1);
  else
    ps.min_h = std::min(ps.min_h, 1);
  return ps;
}

TBTabContainer::TBTabContainer() {
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
  m_root_layout.SetSkinBg(TBIDC("TBTabContainer.rootlayout"));
  m_tab_layout.SetLayoutDistributionPosition(
      LayoutDistributionPosition::kCenter);
  m_tab_layout.SetSkinBg(TBIDC("TBTabContainer.tablayout_x"));
  m_tab_layout.SetLayoutPosition(LayoutPosition::kRightBottom);
  m_content_root.SetGravity(Gravity::kAll);
  m_content_root.SetSkinBg(TBIDC("TBTabContainer.container"));
}

TBTabContainer::~TBTabContainer() {
  m_root_layout.RemoveChild(&m_content_root);
  m_root_layout.RemoveChild(&m_tab_layout);
  RemoveChild(&m_root_layout);
}

void TBTabContainer::SetAxis(Axis axis) {
  m_root_layout.SetAxis(axis);
  m_tab_layout.SetAxis(axis == Axis::kX ? Axis::kY : Axis::kX);
  m_tab_layout.SetSkinBg(axis == Axis::kX
                             ? TBIDC("TBTabContainer.tablayout_y")
                             : TBIDC("TBTabContainer.tablayout_x"));
}

void TBTabContainer::SetValue(int index) {
  if (index == m_current_page) return;
  m_current_page = index;

  // Update the pages visibility and tabs pressed value.
  index = 0;
  TBWidget* page = m_content_root.GetFirstChild();
  TBWidget* tab = m_tab_layout.GetFirstChild();
  for (; page && tab; page = page->GetNext(), tab = tab->GetNext(), index++) {
    bool active = index == m_current_page;
    page->SetVisibilility(active ? Visibility::kVisible
                                 : Visibility::kInvisible);
    tab->SetValue(active ? 1 : 0);
  }
}

int TBTabContainer::GetNumPages() {
  int count = 0;
  for (TBWidget* tab = m_tab_layout.GetFirstChild(); tab; tab = tab->GetNext())
    count++;
  return count;
}

TBWidget* TBTabContainer::GetCurrentPageWidget() const {
  return m_content_root.GetChildFromIndex(m_current_page);
}

void TBTabContainer::SetAlignment(Align align) {
  bool horizontal = (align == Align::kTop || align == Align::kBottom);
  bool reverse = (align == Align::kTop || align == Align::kLeft);
  SetAxis(horizontal ? Axis::kY : Axis::kX);
  m_root_layout.SetLayoutOrder(reverse ? LayoutOrder::kTopToBottom
                                       : LayoutOrder::kBottomToTop);
  m_tab_layout.SetLayoutPosition(reverse ? LayoutPosition::kRightBottom
                                         : LayoutPosition::kLeftTop);
  m_align = align;
}

bool TBTabContainer::OnEvent(const TBWidgetEvent& ev) {
  if ((ev.type == EventType::kClick || ev.type == EventType::kPointerDown) &&
      ev.target->GetID() == TBIDC("tab") &&
      ev.target->GetParent() == &m_tab_layout) {
    int clicked_index = m_tab_layout.GetIndexFromChild(ev.target);
    SetValue(clicked_index);
    return true;
  }
  return false;
}

void TBTabContainer::OnProcess() {
  if (m_need_page_update) {
    m_need_page_update = false;
    // Force update value
    int current_page = m_current_page;
    m_current_page = -1;
    SetValue(current_page);
  }
}

}  // namespace tb
