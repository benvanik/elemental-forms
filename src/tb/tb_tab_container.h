/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TAB_CONTAINER_H
#define TB_TAB_CONTAINER_H

#include "tb_widgets_common.h"

namespace tb {

// A Layout used in TabContainer to apply some default properties on any
// Button added to it.
class TabLayout : public Layout {
 public:
  TBOBJECT_SUBCLASS(TabLayout, Layout);

  void OnChildAdded(Element* child) override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;
};

// A container with tabs for multiple pages.
class TabContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(TabContainer, Element);
  static void RegisterInflater();

  TabContainer();
  ~TabContainer() override;

  // Sets along which axis the content should layouted.
  // Use SetAlignment instead for more choice! Also, calling
  // SetAxis directly does not update the current alignment.
  void SetAxis(Axis axis) override;
  Axis GetAxis() const override { return m_root_layout.GetAxis(); }

  // Sets alignment of the tabs.
  void SetAlignment(Align align);
  Align GetAlignment() const { return m_align; }

  // Sets which page should be selected and visible.
  void SetValue(int value) override;
  int GetValue() override { return m_current_page; }

  // Sets which page should be selected and visible.
  void SetCurrentPage(int index) { SetValue(index); }
  int GetCurrentPage() { return GetValue(); }
  int GetNumPages();

  // Returns the element that is the current page, or nullptr if none is active.
  Element* GetCurrentPageElement() const;

  void OnInflate(const resources::InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
  void OnProcess() override;

  Element* GetContentRoot() override { return &m_content_root; }
  Layout* GetTabLayout() { return &m_tab_layout; }

 protected:
  Layout m_root_layout;
  TabLayout m_tab_layout;
  Element m_content_root;
  bool m_need_page_update = true;
  int m_current_page = 0;
  Align m_align = Align::kTop;
};

}  // namespace tb

#endif  // TB_TAB_CONTAINER_H
