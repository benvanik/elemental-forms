/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_POPUP_WINDOW_H
#define TB_POPUP_WINDOW_H

#include "tb_widgets_listener.h"
#include "tb_window.h"

namespace tb {

// Describes the preferred alignment of a popup relative to a target widget or a
// given point.
// It calculates the rect to be used to match these preferences for any given
// popup and target.
class PopupAlignment {
 public:
  static const int kUnspecified = kInvalidDimension;

  // Aligns relative to the target widget.
  PopupAlignment(Align align = Align::kBottom)
      : pos_in_root(kUnspecified, kUnspecified),
        align(align),
        expand_to_target_width(true) {}

  // Aligns relative to the given position (coordinates relative to the root
  // widget).
  PopupAlignment(const Point& pos_in_root, Align align = Align::kBottom)
      : pos_in_root(pos_in_root), align(align), expand_to_target_width(true) {}

  // Aligns relative to the given position (coordinates relative to the root
  // widget). Applies an additional offset.
  PopupAlignment(const Point& pos_in_root, const Point& pos_offset)
      : pos_in_root(pos_in_root),
        pos_offset(pos_offset),
        align(Align::kBottom),
        expand_to_target_width(true) {}

  // Calculates a good rect for the given popup window using its preferred size
  // and the preferred alignment information stored in this class.
  Rect GetAlignedRect(Widget* popup, Widget* target) const;

  Point pos_in_root;
  Point pos_offset;

  Align align;
  // If true the width of the popup will be at least the same as the target
  // widget if the alignment is Align::kTop or Align::kBottom.
  bool expand_to_target_width;
};

// A popup window that redirects any child widgets events through the given
// target. It will automatically close on click events that are not sent through
// this popup.
class PopupWindow : public Window, private WidgetListener {
 public:
  TBOBJECT_SUBCLASS(PopupWindow, Window);

  PopupWindow(Widget* target);
  ~PopupWindow() override;

  bool Show(const PopupAlignment& alignment);

  Widget* GetEventDestination() override { return m_target.Get(); }

  bool OnEvent(const WidgetEvent& ev) override;

  const WeakWidgetPointer& GetTargetWidget() { return m_target; }

 private:
  void OnWidgetFocusChanged(Widget* widget, bool focused) override;
  bool OnWidgetInvokeEvent(Widget* widget, const WidgetEvent& ev) override;
  void OnWidgetDelete(Widget* widget) override;
  bool OnWidgetDying(Widget* widget) override;

  WeakWidgetPointer m_target;
};

}  // namespace tb

#endif  // TB_POPUP_WINDOW_H
