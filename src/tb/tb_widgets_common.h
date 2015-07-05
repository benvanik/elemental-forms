/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_WIDGETS_COMMON_H
#define TB_WIDGETS_COMMON_H

#include "tb_layout.h"
#include "tb_msg.h"

#include "tb/element.h"
#include "tb/types.h"

namespace tb {

// TextAlign specifies horizontal text alignment.
enum class TextAlign {
  kLeft,
  kRight,
  kCenter,
};
MAKE_ORDERED_ENUM_STRING_UTILS(TextAlign, "left", "right", "center");

// ElementString holds a string that can be painted as one line with the set
// alignment.
class ElementString {
 public:
  ElementString();

  void Paint(Element* element, const Rect& rect, const Color& color);

  int GetWidth(Element* element);
  int GetHeight(Element* element);

  void SetText(const char* text) { m_text = text; }
  std::string GetText() const { return m_text; }

  bool empty() const { return m_text.empty(); }

  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  void SetTextAlign(TextAlign align) { m_text_align = align; }
  TextAlign GetTextAlign() { return m_text_align; }

 public:
  std::string m_text;
  TextAlign m_text_align = TextAlign::kCenter;
};

// A one line text field that is not editable.
class Label : public Element {
 public:
  TBOBJECT_SUBCLASS(Label, Element);
  static void RegisterInflater();

  Label();

  // Sets the text of the text field.
  void SetText(const char* text) override;
  using Element::SetText;
  std::string GetText() override { return m_text.GetText(); }

  bool empty() const { return m_text.empty(); }

  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  void SetTextAlign(TextAlign align) { m_text.SetTextAlign(align); }
  TextAlign GetTextAlign() { return m_text.GetTextAlign(); }

  // Sets if this text field should be allowed to squeeze below its preferred
  // size. If squeezable it may shrink to width 0.
  void SetSqueezable(bool squeezable);
  bool GetSqueezable() { return m_squeezable; }

  void OnInflate(const parsing::InflateInfo& info) override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;
  void OnFontChanged() override;
  void OnPaint(const PaintProps& paint_props) override;

 protected:
  // This value on m_cached_text_width means it needs to be updated again.
  static const int kTextWidthCacheNeedsUpdate = -1;

  ElementString m_text;
  int m_cached_text_width = kTextWidthCacheNeedsUpdate;
  bool m_squeezable = false;
};

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

  class ButtonLayout : public Layout {
    void OnChildAdded(Element* child) override;
    void OnChildRemove(Element* child) override;
  };

  ButtonLayout m_layout;
  Label m_textfield;
  bool m_auto_repeat_click = false;
  bool m_toggle_mode = false;
};

// Has a text field in its internal layout by default. Pointer input on the text
// field will be redirected to another child element (that you add) to it.
// Typically useful for creating check boxes, radio buttons with labels.
class LabelContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(LabelContainer, Element);
  static void RegisterInflater();

  LabelContainer();
  ~LabelContainer() override;

  // Sets along which axis the content should layout (if the label has more
  // content than the text).
  void SetAxis(Axis axis) override { m_layout.SetAxis(axis); }
  Axis GetAxis() const override { return m_layout.GetAxis(); }

  // Sets the text of the label.
  void SetText(const char* text) override { m_textfield.SetText(text); }
  std::string GetText() override { return m_textfield.GetText(); }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override {
    return m_layout.GetPreferredSize();
  }

  Element* GetContentRoot() override { return &m_layout; }

  bool OnEvent(const ElementEvent& ev) override;

 protected:
  Layout m_layout;
  Label m_textfield;
};

// A element showing a skin element, constrained in size to its skin.
// If you need to load and show images dynamically (i.e. not always loaded as
// the skin), you can use ImageElement.
class SkinImage : public Element {
 public:
  TBOBJECT_SUBCLASS(SkinImage, Element);
  static void RegisterInflater();

  SkinImage() = default;
  SkinImage(const TBID& skin_bg) { SetSkinBg(skin_bg); }

  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;
};

// A element only showing a skin.
// It is disabled by default.
class Separator : public Element {
 public:
  TBOBJECT_SUBCLASS(Separator, Element);
  static void RegisterInflater();

  Separator();
};

// An animation that is running while its value is 1.
// Typically used to indicate that the application is working.
class ProgressSpinner : public Element, protected MessageHandler {
 public:
  TBOBJECT_SUBCLASS(ProgressSpinner, Element);
  static void RegisterInflater();

  ProgressSpinner();

  // Returns true if the animation is running.
  bool IsRunning() { return m_value > 0; }

  // Begin/End are used to start or stop the animation in a incremental way.
  // If several tasks may activate the same spinner, calling Begin/End instead
  // of using SetValue, so it will keep running as long as any source wants it
  // to.
  void Begin() { SetValue(GetValue() + 1); }
  void End() { SetValue(GetValue() - 1); }

  // Setting the value to 1 will start the spinner. Setting it to 0 will stop
  // it.
  void SetValue(int value) override;
  int GetValue() override { return m_value; }

  void OnPaint(const PaintProps& paint_props) override;

  void OnMessageReceived(Message* msg) override;

 protected:
  // How fast should the spinner animation animate.
  // FIX: Add spin_speed to skin!
  // FIX: Make it post messages only if visible
  static const int kSpinSpeed = 1000 / 30;

  int m_value = 0;
  int m_frame = 0;
  TBID m_skin_fg;
};

// Shared functionality for CheckBox and RadioButton.
class BaseRadioCheckBox : public Element {
 public:
  TBOBJECT_SUBCLASS(BaseRadioCheckBox, Element);

  BaseRadioCheckBox();

  void SetValue(int value) override;
  int GetValue() override { return m_value; }

  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;
  bool OnEvent(const ElementEvent& ev) override;

  // Makes sure all elements sharing the same group as new_leader are set to
  // value 0.
  static void UpdateGroupElements(Element* new_leader);

 protected:
  int m_value = 0;
};

// A box toggling a check mark on click.
// For a labeled checkbox, use a LabelContainer containing a CheckBox.
class CheckBox : public BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(CheckBox, BaseRadioCheckBox);
  static void RegisterInflater();

  CheckBox() { SetSkinBg(TBIDC("CheckBox"), InvokeInfo::kNoCallbacks); }
};

// A button which unselects other radiobuttons of the same group number when
// clicked.
// For a labeled radio button, use a LabelContainer containing a RadioButton.
class RadioButton : public BaseRadioCheckBox {
 public:
  TBOBJECT_SUBCLASS(RadioButton, BaseRadioCheckBox);
  static void RegisterInflater();

  RadioButton() { SetSkinBg(TBIDC("RadioButton"), InvokeInfo::kNoCallbacks); }
};

// A scroll bar in the given axis.
class ScrollBar : public Element {
 public:
  TBOBJECT_SUBCLASS(ScrollBar, Element);
  static void RegisterInflater();

  ScrollBar();
  ~ScrollBar() override;

  // Sets along which axis the scrollbar should scroll.
  void SetAxis(Axis axis) override;
  Axis GetAxis() const override { return m_axis; }

  // Sets the min, max limits for the scrollbar.
  // The visible parameter is how much of the range that is visible.
  // When this is called, the scrollbar might change value and invoke if the
  // current value is outside of the new limits.
  void SetLimits(double min, double max, double visible);

  // Returns true if the scrollbar has anywhere to go with the current limits.
  bool CanScroll() const { return m_visible > 0; }

  // Returns true if the scrollbar can scroll in the positive direction with its
  // current limits.
  bool CanScrollPositive() const { return m_value < m_max; }

  // Returns true if the scrollbar can scroll in the negative direction with its
  // current limits.
  bool CanScrollNegative() const { return m_value > m_min; }

  double GetMinValue() const { return m_min; }
  double GetMaxValue() const { return m_max; }
  double GetVisible() const { return m_visible; }

  // Same as SetValue, but with double precision.
  void SetValueDouble(double value) override;
  double GetValueDouble() override { return m_value; }

  void SetValue(int value) override { SetValueDouble(value); }
  int GetValue() override { return (int)GetValueDouble(); }

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
  void OnResized(int old_w, int old_h) override;

 protected:
  void UpdateHandle();

  Element m_handle;
  Axis m_axis;
  double m_value = 0;
  double m_min = 0;
  double m_max = 1;
  double m_visible = 1;
  double m_to_pixel_factor = 0;
};

// A horizontal or vertical slider for a number within a range.
// FIX: Add a "track value" showing as a line within the track (to be used for
// buffering etc).
// FIX: Also add a auto track that keeps it up to date with value (default).
class Slider : public Element {
 public:
  TBOBJECT_SUBCLASS(Slider, Element);
  static void RegisterInflater();

  Slider();
  ~Slider() override;

  // Sets along which axis the scrollbar should scroll.
  void SetAxis(Axis axis) override;
  Axis GetAxis() const override { return m_axis; }

  // Sets the min, max limits for the slider.
  void SetLimits(double min, double max);

  double GetMinValue() const { return m_min; }
  double GetMaxValue() const { return m_max; }

  // Gets a small value (depending on the min and max limits) for stepping by
  // f.ex. keyboard.
  double GetSmallStep() const { return (m_max - m_min) / 100.0; }

  // Same as SetValue, but with double precision.
  void SetValueDouble(double value) override;
  double GetValueDouble() override { return m_value; }

  void SetValue(int value) override { SetValueDouble(value); }
  int GetValue() override { return (int)GetValueDouble(); }

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
  void OnResized(int old_w, int old_h) override;

 protected:
  Element m_handle;
  Axis m_axis;
  double m_value = 0;
  double m_min = 0;
  double m_max = 1;
  double m_to_pixel_factor = 0;
  void UpdateHandle();
};

// Container is just a Element with border and padding (using skin
// "Container").
class Container : public Element {
 public:
  TBOBJECT_SUBCLASS(Container, Element);
  static void RegisterInflater();

  Container();
};

// Moves its parent element when dragged.
class Mover : public Element {
 public:
  TBOBJECT_SUBCLASS(Mover, Element);
  static void RegisterInflater();

  Mover();

  bool OnEvent(const ElementEvent& ev) override;
};

// A lower right corner resize grip.
// It will resize its parent element.
class Resizer : public Element {
 public:
  TBOBJECT_SUBCLASS(Resizer, Element);
  static void RegisterInflater();

  Resizer();

  HitStatus GetHitStatus(int x, int y) override;
  bool OnEvent(const ElementEvent& ev) override;
};

// Dims elements in the background and blocks input.
class Dimmer : public Element {
 public:
  TBOBJECT_SUBCLASS(Dimmer, Element);
  static void RegisterInflater();

  Dimmer();

  void OnAdded() override;
};

}  // namespace tb

#endif  // TB_WIDGETS_COMMON_H
