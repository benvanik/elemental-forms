/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "el/elements/button.h"
#include "el/elements/tab_container.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void TabContainer::TabLayoutBox::OnChildAdded(Element* child) {
  if (auto button = util::SafeCast<Button>(child)) {
    button->set_squeezable(true);
    button->set_background_skin(TBIDC("TabContainer.tab"));
    button->set_id(TBIDC("tab"));
  }
}

PreferredSize TabContainer::TabLayoutBox::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = LayoutBox::OnCalculatePreferredContentSize(constraints);
  // Make sure the number of tabs doesn't grow parents.
  // It is only the content that should do that. The tabs
  // will scroll anyway.
  if (axis() == Axis::kX) {
    ps.min_w = std::min(ps.min_w, 1);
  } else {
    ps.min_h = std::min(ps.min_h, 1);
  }
  return ps;
}

void TabContainer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(TabContainer, Value::Type::kNull,
                               ElementZ::kTop);
}

TabContainer::TabContainer() {
  AddChild(&m_root_layout);
  // Put the tab layout on top of the content in Z order so their skin can make
  // a seamless overlap over the border. Control which side they are layouted
  // to by calling set_layout_order.
  m_root_layout.AddChild(&m_content_root);
  m_root_layout.AddChild(&m_tab_bar);
  m_root_layout.set_axis(Axis::kY);
  m_root_layout.set_gravity(Gravity::kAll);
  m_root_layout.set_layout_distribution(LayoutDistribution::kAvailable);
  m_root_layout.set_layout_order(LayoutOrder::kTopToBottom);
  m_root_layout.set_background_skin(TBIDC("TabContainer.rootlayout"));
  m_tab_bar.set_layout_distribution_position(
      LayoutDistributionPosition::kCenter);
  m_tab_bar.set_background_skin(TBIDC("TabContainer.tablayout_x"));
  m_tab_bar.set_layout_position(LayoutPosition::kRightBottom);
  m_content_root.set_gravity(Gravity::kAll);
  m_content_root.set_background_skin(TBIDC("TabContainer.container"));
}

TabContainer::~TabContainer() {
  m_root_layout.RemoveChild(&m_content_root);
  m_root_layout.RemoveChild(&m_tab_bar);
  RemoveChild(&m_root_layout);
}

void TabContainer::OnInflate(const parsing::InflateInfo& info) {
  Element::OnInflate(info);

  if (const char* align = info.node->GetValueString("align", nullptr)) {
    set_alignment(el::from_string(align, alignment()));
  }
  // Allow additional attributes to be specified for the "tabs", "content" and
  // "root" layouts by calling OnInflate.
  if (auto tabs = info.node->GetNode("tabs")) {
    // Inflate the tabs elements into the tab layout.
    info.reader->LoadNodeTree(tab_bar(), tabs);

    parsing::InflateInfo inflate_info(info.reader, tab_bar()->content_root(),
                                      tabs, Value::Type::kNull);
    tab_bar()->OnInflate(inflate_info);
  }
  if (auto tabs = info.node->GetNode("content")) {
    parsing::InflateInfo inflate_info(info.reader, content_root(), tabs,
                                      Value::Type::kNull);
    content_root()->OnInflate(inflate_info);
  }
  if (auto tabs = info.node->GetNode("root")) {
    parsing::InflateInfo inflate_info(info.reader, &m_root_layout, tabs,
                                      Value::Type::kNull);
    m_root_layout.OnInflate(inflate_info);
  }
}

void TabContainer::set_axis(Axis axis) {
  m_root_layout.set_axis(axis);
  m_tab_bar.set_axis(axis == Axis::kX ? Axis::kY : Axis::kX);
  m_tab_bar.set_background_skin(axis == Axis::kX
                                    ? TBIDC("TabContainer.tablayout_y")
                                    : TBIDC("TabContainer.tablayout_x"));
}

void TabContainer::set_value(int index) {
  if (index == m_current_page) return;
  m_current_page = index;

  // Update the pages visibility and tabs pressed value.
  index = 0;
  Element* page = m_content_root.first_child();
  Element* tab = m_tab_bar.first_child();
  for (; page && tab; page = page->GetNext(), tab = tab->GetNext(), index++) {
    bool active = index == m_current_page;
    page->set_visibility(active ? Visibility::kVisible
                                : Visibility::kInvisible);
    tab->set_value(active ? 1 : 0);
  }
}

int TabContainer::page_count() {
  int count = 0;
  for (Element* tab = m_tab_bar.first_child(); tab; tab = tab->GetNext()) {
    count++;
  }
  return count;
}

Element* TabContainer::current_page_element() const {
  return m_content_root.GetChildFromIndex(m_current_page);
}

void TabContainer::set_alignment(Align align) {
  bool horizontal = (align == Align::kTop || align == Align::kBottom);
  bool reverse = (align == Align::kTop || align == Align::kLeft);
  set_axis(horizontal ? Axis::kY : Axis::kX);
  m_root_layout.set_layout_order(reverse ? LayoutOrder::kTopToBottom
                                         : LayoutOrder::kBottomToTop);
  m_tab_bar.set_layout_position(reverse ? LayoutPosition::kRightBottom
                                        : LayoutPosition::kLeftTop);
  m_align = align;
}

bool TabContainer::OnEvent(const Event& ev) {
  if ((ev.type == EventType::kClick || ev.type == EventType::kPointerDown) &&
      ev.target->id() == TBIDC("tab") && ev.target->parent() == &m_tab_bar) {
    int clicked_index = m_tab_bar.GetIndexFromChild(ev.target);
    set_value(clicked_index);
    return true;
  }
  return Element::OnEvent(ev);
}

void TabContainer::OnProcess() {
  if (m_need_page_update) {
    m_need_page_update = false;
    // Force update value.
    int current_page = m_current_page;
    m_current_page = -1;
    set_value(current_page);
  }
}

}  // namespace elements
}  // namespace el
