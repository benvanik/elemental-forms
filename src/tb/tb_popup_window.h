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

/** TBPopupAlignment describes the preferred alignment of a popup
        relative to a target widget or a given point.

        It calculates the rect to be used to match these preferences
        for any given popup and target. */
class TBPopupAlignment {
 public:
  static const int kUnspecified = kInvalidDimension;

  /** Align relative to the target widget. */
  TBPopupAlignment(Align align = Align::kBottom)
      : pos_in_root(kUnspecified, kUnspecified),
        align(align),
        expand_to_target_width(true) {}

  /** Align relative to the given position (coordinates relative to the root
   * widget). */
  TBPopupAlignment(const TBPoint& pos_in_root, Align align = Align::kBottom)
      : pos_in_root(pos_in_root), align(align), expand_to_target_width(true) {}

  /** Align relative to the given position (coordinates relative to the root
     widget).
          Applies an additional offset. */
  TBPopupAlignment(const TBPoint& pos_in_root, const TBPoint& pos_offset)
      : pos_in_root(pos_in_root),
        pos_offset(pos_offset),
        align(Align::kBottom),
        expand_to_target_width(true) {}

  /** Calculate a good rect for the given popup window using its preferred size
     and
          the preferred alignment information stored in this class. */
  TBRect GetAlignedRect(TBWidget* popup, TBWidget* target) const;

  TBPoint pos_in_root;
  TBPoint pos_offset;

  Align align;
  /** If true, the width of the popup will be at least the same as the target
     widget
          if the alignment is Align::kTop or Align::kBottom. */
  bool expand_to_target_width;
};

/** TBPopupWindow is a popup window that redirects any child widgets events
        through the given target. It will automatically close on click events
   that
        are not sent through this popup. */

class TBPopupWindow : public TBWindow, private TBWidgetListener {
 public:
  TBOBJECT_SUBCLASS(TBPopupWindow, TBWindow);

  TBPopupWindow(TBWidget* target);
  ~TBPopupWindow();

  bool Show(const TBPopupAlignment& alignment);

  virtual TBWidget* GetEventDestination() { return m_target.Get(); }

  virtual bool OnEvent(const TBWidgetEvent& ev);

  const TBWidgetSafePointer& GetTargetWidget() { return m_target; }

 private:
  TBWidgetSafePointer m_target;
  // TBWidgetListener
  virtual void OnWidgetFocusChanged(TBWidget* widget, bool focused);
  virtual bool OnWidgetInvokeEvent(TBWidget* widget, const TBWidgetEvent& ev);
  virtual void OnWidgetDelete(TBWidget* widget);
  virtual bool OnWidgetDying(TBWidget* widget);
};

}  // namespace tb

#endif  // TB_POPUP_WINDOW_H
