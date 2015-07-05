/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_TOGGLE_CONTAINER_H_
#define TB_ELEMENTS_TOGGLE_CONTAINER_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// Defines what should toggle when the value changes.
enum class ToggleAction {
  kNothing,   // Nothing happens (the default).
  kEnabled,   // Enabled/disabled state.
  kOpacity,   // Opacity 1/0.
  kExpanded,  // Expanded/collapsed (In parent axis direction).
};
MAKE_ORDERED_ENUM_STRING_UTILS(ToggleAction, "nothing", "enabled", "opacity",
                               "expanded");

// A element that toggles a property when its value change between 0 and 1.
// ToggleAction specifies what property will toggle.
// This is useful f.ex to toggle a whole group of child elements depending on
// the value of some other element. By connecting the ToggleContainer with a
// element connection, this can happen completly automatically.
class ToggleContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(ToggleContainer, Element);
  static void RegisterInflater();

  ToggleContainer();

  ToggleAction toggle_action() const { return m_toggle; }
  void set_toggle_action(ToggleAction toggle);

  bool is_inverted() const { return m_invert; }
  // Sets if the toggle state should be inverted.
  void set_inverted(bool invert);

  // Gets the current value, after checking the invert mode.
  bool is_toggled() const { return m_invert ? !m_value : !!m_value; }

  int value() override { return m_value; }
  // Sets the value of this element.
  // 1 will turn on the toggle, 0 will turn it off (or the opposite if the
  // invert mode is set).
  void set_value(int value) override;

  void OnInflate(const parsing::InflateInfo& info) override;

 private:
  void UpdateInternal();

  ToggleAction m_toggle = ToggleAction::kNothing;
  bool m_invert = false;
  int m_value = 0;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_TOGGLE_CONTAINER_H_
