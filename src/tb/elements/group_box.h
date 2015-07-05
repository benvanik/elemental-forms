/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_GROUP_BOX_H_
#define TB_ELEMENTS_GROUP_BOX_H_

#include "tb/element.h"
#include "tb/elements/button.h"
#include "tb/elements/layout_box.h"
#include "tb/elements/toggle_container.h"

namespace tb {
namespace elements {

// A element with a header that when clicked toggles its children on and off
// (using a internal ToggleContainer with ToggleAction::kExpanded).
// The header is a GroupBoxHeader.
// The skin names of the internal elements are:
//     GroupBox           - This element itself.
//     GroupBox.layout    - The layout that wraps the header and the container.
//     GroupBox.container - The toggle container with the children that
//                          expands/collapses.
class GroupBox : public Element {
  class Header;

 public:
  TBOBJECT_SUBCLASS(GroupBox, Element);
  static void RegisterInflater();

  GroupBox();
  ~GroupBox() override;

  Element* GetHeader() { return &m_header; }
  ToggleContainer* GetContainer() { return &m_toggle_container; }

  // Sets if the section should be scrolled into view after next layout.
  void SetPendingScrollIntoView(bool pending_scroll) {
    m_pending_scroll = pending_scroll;
  }

  // Sets the text of the text field.
  void SetText(const char* text) override { m_header.SetText(text); }
  std::string GetText() override { return m_header.GetText(); }

  void SetValue(int value) override;
  int GetValue() override { return m_toggle_container.GetValue(); }

  Element* GetContentRoot() override {
    return m_toggle_container.GetContentRoot();
  }
  void OnProcessAfterChildren() override;

  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;

 private:
  // Just a thin wrapper for a Button that is in toggle mode with the skin
  // GroupBoxHeader by default.
  // It is used as the clickable header in GroupBox that toggles the section.
  class Header : public Button {
   public:
    TBOBJECT_SUBCLASS(Header, Button);
    Header();
    bool OnEvent(const ElementEvent& ev) override;
  };

  LayoutBox m_layout;
  Header m_header;
  ToggleContainer m_toggle_container;
  bool m_pending_scroll = false;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_GROUP_BOX_H_
