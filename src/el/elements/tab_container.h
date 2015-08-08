/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_TAB_CONTAINER_H_
#define EL_ELEMENTS_TAB_CONTAINER_H_

#include "el/element.h"
#include "el/elements/layout_box.h"

namespace el {
namespace elements {

// A container with tabs for multiple pages.
class TabContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(TabContainer, Element);
  static void RegisterInflater();

  TabContainer();
  ~TabContainer() override;

  Axis axis() const override { return m_root_layout.axis(); }
  // Sets along which axis the content should layouted.
  // Use SetAlignment instead for more choice! Also, calling
  // SetAxis directly does not update the current alignment.
  void set_axis(Axis axis) override;

  Align alignment() const { return m_align; }
  // Sets alignment of the tabs.
  void set_alignment(Align align);

  int value() override { return m_current_page; }
  // Sets which page should be selected and visible.
  void set_value(int value) override;

  int page_count();
  int current_page() { return value(); }
  // Sets which page should be selected and visible.
  void set_current_page(int index) { set_value(index); }

  // Returns the element that is the current page, or nullptr if none is active.
  Element* current_page_element() const;

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const Event& ev) override;
  void OnProcess() override;

  Element* content_root() override { return &m_content_root; }
  LayoutBox* tab_bar() { return &m_tab_bar; }

 protected:
  // A Box used in TabContainer to apply some default properties on any
  // Button added to it.
  class TabLayoutBox : public LayoutBox {
   public:
    TBOBJECT_SUBCLASS(TabLayoutBox, LayoutBox);

    void OnChildAdded(Element* child) override;
    PreferredSize OnCalculatePreferredContentSize(
        const SizeConstraints& constraints) override;
  };

  LayoutBox m_root_layout;
  TabLayoutBox m_tab_bar;
  Element m_content_root;
  bool m_need_page_update = true;
  int m_current_page = 0;
  Align m_align = Align::kTop;
};

}  // namespace elements
namespace dsl {

struct TabContainerNode : public ElementNode<TabContainerNode> {
  using R = TabContainerNode;
  TabContainerNode(std::vector<Node> children = {})
      : ElementNode("TabContainer", {}, std::move(children)) {}
  //
  R& value(int32_t value) {
    set("value", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& align(Align value) {
    set("align", el::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
  // content?
  // root?
  TabContainerNode& tab(Node tab_button, Node tab_content) {
    auto tabs_node = GetOrCreateNode("tabs");
    tabs_node->Add(tab_button.parse_node());
    parse_node_->Add(tab_content.parse_node());
    return *this;
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_TAB_CONTAINER_H_
