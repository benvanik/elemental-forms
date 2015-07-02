/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_TOGGLE_CONTAINER_H
#define TB_TOGGLE_CONTAINER_H

#include "tb_widgets_common.h"

namespace tb {

// Defines what should toggle when the value changes.
enum class ToggleAction {
  kNothing,   // Nothing happens (the default).
  kEnabled,   // Enabled/disabled state.
  kOpacity,   // Opacity 1/0.
  kExpanded,  // Expanded/collapsed (In parent axis direction).
};
MAKE_ORDERED_ENUM_STRING_UTILS(ToggleAction, "nothing", "enabled", "opacity",
                               "expanded");

// A widget that toggles a property when its value change between 0 and 1.
// ToggleAction specifies what property will toggle.
// This is useful f.ex to toggle a whole group of child widgets depending on the
// value of some other widget. By connecting the ToggleContainer with a widget
// connection, this can happen completly automatically.
class ToggleContainer : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(ToggleContainer, TBWidget);

  ToggleContainer();

  void SetToggleAction(ToggleAction toggle);
  ToggleAction GetToggleAction() const { return m_toggle; }

  // Sets if the toggle state should be inverted.
  void SetInvert(bool invert);
  bool GetInvert() const { return m_invert; }

  // Gets the current value, after checking the invert mode.
  bool GetIsOn() const { return m_invert ? !m_value : !!m_value; }

  // Sets the value of this widget.
  // 1 will turn on the toggle, 0 will turn it off (or the opposite if the
  // invert mode is set).
  void SetValue(int value) override;
  int GetValue() override { return m_value; }

  void OnInflate(const INFLATE_INFO& info) override;

 private:
  void UpdateInternal();

  ToggleAction m_toggle = ToggleAction::kNothing;
  bool m_invert = false;
  int m_value = 0;
};

// Just a thin wrapper for a Button that is in toggle mode with the skin
// SectionHeader by default.
// It is used as the clickable header in Section that toggles the section.
class SectionHeader : public Button {
 public:
  TBOBJECT_SUBCLASS(SectionHeader, Button);

  SectionHeader();

  bool OnEvent(const TBWidgetEvent& ev) override;
};

// A widget with a header that when clicked toggles its children on and off
// (using a internal ToggleContainer with ToggleAction::kExpanded).
// The header is a SectionHeader.
// The skin names of the internal widgets are:
//     Section           - This widget itself.
//     Section.layout    - The layout that wraps the header and the container.
//     Section.container - The toggle container with the children that
//                         expands/collapses.
class Section : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(Section, TBWidget);

  Section();
  ~Section() override;

  TBLayout* GetLayout() { return &m_layout; }
  SectionHeader* GetHeader() { return &m_header; }
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

  TBWidget* GetContentRoot() override {
    return m_toggle_container.GetContentRoot();
  }
  void OnProcessAfterChildren() override;

  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;

 private:
  TBLayout m_layout;
  SectionHeader m_header;
  ToggleContainer m_toggle_container;
  bool m_pending_scroll = false;
};

}  // namespace tb

#endif  // TB_TOGGLE_CONTAINER_H
