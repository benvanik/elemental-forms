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

/** TBToggleContainer is a widget that toggles a property when its value
        change between 0 and 1. TOGGLE specifies what property will toggle.
        This is useful f.ex to toggle a whole group of child widgets depending
        on the value of some other widget. By connecting the TBToggleContainer
        with a widget connection, this can happen completly automatically. */
class TBToggleContainer : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(TBToggleContainer, TBWidget);

  TBToggleContainer();

  /** Set what should toggle when the value changes. */
  void SetToggleAction(ToggleAction toggle);
  ToggleAction GetToggleAction() const { return m_toggle; }

  /** Set if the toggle state should be inverted. */
  void SetInvert(bool invert);
  bool GetInvert() const { return m_invert; }

  /** Get the current value, after checking the invert mode. */
  bool GetIsOn() const { return m_invert ? !m_value : !!m_value; }

  /** Set the value of this widget. 1 will turn on the toggle, 0 will turn it
     off (or
          the opposite if the invert mode is set). */
  virtual void SetValue(int value);
  virtual int GetValue() { return m_value; }

  virtual void OnInflate(const INFLATE_INFO& info);

 private:
  void UpdateInternal();
  ToggleAction m_toggle;
  bool m_invert;
  int m_value;
};

/** TBSectionHeader is just a thin wrapper for a TBButton that is in toggle
        mode with the skin TBSectionHeader by default. It is used as the
   clickable
        header in TBSection that toggles the section. */
class TBSectionHeader : public TBButton {
 public:
  TBOBJECT_SUBCLASS(TBSectionHeader, TBButton);

  TBSectionHeader();

  virtual bool OnEvent(const TBWidgetEvent& ev);
};

/** TBSection is a widget with a header that when clicked toggles its children
        on and off (using a internal TBToggleContainer with
   ToggleAction::kExpanded).

        The header is a TBSectionHeader.

        The skin names of the internal widgets are:
                TBSection				- This widget itself.
                TBSection.layout		- The layout that wraps the
   header and the container.
                TBSection.container		- The toggle container with
   the children that expands/collapses.
*/

class TBSection : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(TBSection, TBWidget);

  TBSection();
  ~TBSection();

  TBLayout* GetLayout() { return &m_layout; }
  TBSectionHeader* GetHeader() { return &m_header; }
  TBToggleContainer* GetContainer() { return &m_toggle_container; }

  /** Set if the section should be scrolled into view after next layout. */
  void SetPendingScrollIntoView(bool pending_scroll) {
    m_pending_scroll = pending_scroll;
  }

  /** Set the text of the text field. */
  void SetText(const char* text) override { m_header.SetText(text); }
  std::string GetText() override { return m_header.GetText(); }

  virtual void SetValue(int value);
  virtual int GetValue() { return m_toggle_container.GetValue(); }

  virtual TBWidget* GetContentRoot() {
    return m_toggle_container.GetContentRoot();
  }
  virtual void OnProcessAfterChildren();

  virtual PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints);

 private:
  TBLayout m_layout;
  TBSectionHeader m_header;
  TBToggleContainer m_toggle_container;
  bool m_pending_scroll;
};

}  // namespace tb

#endif  // TB_TOGGLE_CONTAINER_H
