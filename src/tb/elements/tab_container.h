/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_TAB_CONTAINER_H_
#define TB_ELEMENTS_TAB_CONTAINER_H_

#include "tb/element.h"
#include "tb/elements/layout_box.h"

namespace tb {
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
  bool OnEvent(const ElementEvent& ev) override;
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
}  // namespace tb

#endif  // TB_ELEMENTS_TAB_CONTAINER_H_
