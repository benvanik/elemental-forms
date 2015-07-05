/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_BUTTON_H_
#define TB_ELEMENTS_BUTTON_H_

#include "tb/element.h"
#include "tb/elements/label.h"
#include "tb/elements/layout_box.h"
#include "tb/message_handler.h"
#include "tb/types.h"

namespace tb {
namespace elements {

// A regular button element.
// Has a text field in its internal layout by default. Other elements can be
// added under GetContentRoot().
class Button : public Element, protected MessageHandler {
 public:
  TBOBJECT_SUBCLASS(Button, Element);
  static void RegisterInflater();

  Button();
  ~Button() override;

  // Sets along which axis the content should layout (if the button has more
  // content than the text).
  void SetAxis(Axis axis) override { m_layout.SetAxis(axis); }
  Axis GetAxis() const override { return m_layout.GetAxis(); }

  // Sets if the text field should be allowed to squeeze below its preferred
  // size. If squeezable it may shrink to width 0.
  void SetSqueezable(bool squeezable) { m_textfield.SetSqueezable(squeezable); }
  bool GetSqueezable() { return m_textfield.GetSqueezable(); }

  // Sets if the button should fire repeatedly while pressed.
  void SetAutoRepeat(bool auto_repeat_click) {
    m_auto_repeat_click = auto_repeat_click;
  }
  bool GetAutoRepeat() { return m_auto_repeat_click; }

  // Sets if the button should toggle on and off, instead of just fire click
  // events. When it's on, it will have value 1 pressed state.
  void SetToggleMode(bool toggle_mode_on) { m_toggle_mode = toggle_mode_on; }
  bool GetToggleMode() const { return m_toggle_mode; }

  // Sets the text of the button.
  void SetText(const char* text) override;
  using Element::SetText;
  std::string GetText() override { return m_textfield.GetText(); }

  void SetValue(int value) override;
  int GetValue() override;

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnCaptureChanged(bool captured) override;
  void OnSkinChanged() override;
  bool OnEvent(const ElementEvent& ev) override;
  HitStatus GetHitStatus(int x, int y) override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override {
    return m_layout.GetPreferredSize();
  }

  Element* GetContentRoot() override { return &m_layout; }

  void OnMessageReceived(Message* msg) override;

 protected:
  static const int kAutoClickFirstDelayMillis = 500;
  static const int kAutoClickRepeattDelayMillis = 100;

  void UpdateLabelVisibility();
  bool CanToggle() { return m_toggle_mode || GetGroupID(); }

  class ButtonLayoutBox : public LayoutBox {
    void OnChildAdded(Element* child) override;
    void OnChildRemove(Element* child) override;
  };

  ButtonLayoutBox m_layout;
  Label m_textfield;
  bool m_auto_repeat_click = false;
  bool m_toggle_mode = false;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_BUTTON_H_
