/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_GROUP_BOX_H_
#define EL_ELEMENTS_GROUP_BOX_H_

#include <string>

#include "el/element.h"
#include "el/elements/button.h"
#include "el/elements/layout_box.h"
#include "el/elements/toggle_container.h"

namespace el {
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

  Element* header_element() { return &m_header; }
  ToggleContainer* container() { return &m_toggle_container; }

  // Sets if the section should be scrolled into view after next layout.
  void set_pending_scroll_into_view(bool pending_scroll) {
    m_pending_scroll = pending_scroll;
  }

  std::string text() override { return m_header.text(); }
  // Sets the text of the text field.
  void set_text(const char* text) override { m_header.set_text(text); }

  int value() override { return m_toggle_container.value(); }
  void set_value(int value) override;

  Element* content_root() override { return m_toggle_container.content_root(); }
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
    bool OnEvent(const Event& ev) override;
  };

  LayoutBox m_layout;
  Header m_header;
  ToggleContainer m_toggle_container;
  bool m_pending_scroll = false;
};

}  // namespace elements
namespace dsl {

struct GroupBoxNode : public ElementNode<GroupBoxNode> {
  using R = GroupBoxNode;
  explicit GroupBoxNode(const char* text = nullptr) : ElementNode("GroupBox") {
    if (text) {
      this->text(text);
    }
  }
  R& content(Node child) { return this->child(child); }
  //
  R& value(int32_t value) {
    set("value", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& text(std::string value) {
    set("text", value);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_GROUP_BOX_H_
