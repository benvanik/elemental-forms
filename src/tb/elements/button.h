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
// added under content_root().
class Button : public Element, protected MessageHandler {
 public:
  TBOBJECT_SUBCLASS(Button, Element);
  static void RegisterInflater();

  Button();
  ~Button() override;

  Axis axis() const override { return m_layout.axis(); }
  // Sets along which axis the content should layout (if the button has more
  // content than the text).
  void set_axis(Axis axis) override { m_layout.set_axis(axis); }

  bool is_squeezable() { return m_textfield.is_squeezable(); }
  // Sets if the text field should be allowed to squeeze below its preferred
  // size. If squeezable it may shrink to width 0.
  void set_squeezable(bool squeezable) {
    m_textfield.set_squeezable(squeezable);
  }

  bool is_auto_repeat() { return m_auto_repeat_click; }
  // Sets if the button should fire repeatedly while pressed.
  void set_auto_repeat(bool auto_repeat_click) {
    m_auto_repeat_click = auto_repeat_click;
  }

  bool is_toggle_mode() const { return m_toggle_mode; }
  // Sets if the button should toggle on and off, instead of just fire click
  // events. When it's on, it will have value 1 pressed state.
  void set_toggle_mode(bool toggle_mode_on) { m_toggle_mode = toggle_mode_on; }

  std::string text() override { return m_textfield.text(); }
  // Sets the text of the button.
  void set_text(const char* text) override;
  using Element::set_text;

  int value() override;
  void set_value(int new_value) override;

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnCaptureChanged(bool captured) override;
  void OnSkinChanged() override;
  bool OnEvent(const ElementEvent& ev) override;
  HitStatus GetHitStatus(int x, int y) override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override {
    return m_layout.GetPreferredSize();
  }

  Element* content_root() override { return &m_layout; }

  void OnMessageReceived(Message* msg) override;

 protected:
  static const int kAutoClickFirstDelayMillis = 500;
  static const int kAutoClickRepeattDelayMillis = 100;

  void UpdateLabelVisibility();
  bool can_toggle() { return m_toggle_mode || group_id(); }

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
