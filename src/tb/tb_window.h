/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WINDOW_H
#define TB_WINDOW_H

#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"

namespace tb {

enum class WindowSettings {
  kNone = 0,         // Chrome less window without any other settings.
  kTitleBar = 1,     // Show a title bar that can also move the window.
  kResizable = 2,    // Show a widget for resizing the window.
  kCloseButton = 4,  // Show a widget for closing the window.
  kCanActivate = 8,  // Can be activated and deactivate other windows.

  kDefault = kTitleBar | kResizable | kCloseButton | kCanActivate,
};
MAKE_ENUM_FLAG_COMBO(WindowSettings);

// A TBWidget that provides some window-like features.
// It can have a titlebar, be movable, resizable etc.
// It will activate and deactivate other windows on click (which will restore
// focus to the last focused child widget).
class Window : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(Window, TBWidget);

  Window();
  ~Window() override;

  // Closes this window.
  // NOTE: This window will be deleted after this call!
  void Close();

  // Returns true if this window is active.
  bool IsActive() const;

  // Activates this window if it's not already activated.
  // This will deactivate any currently activated window..
  // This will automatically call EnsureFocus to restore/set focus to this
  // window.
  void Activate();

  // Ensures that this window has focus by attempting to find a focusable child
  // widget.
  // It will first try to restore focus to the last focused widget in this
  // window, or a widget that has received SetFocus while the window was
  // inactive. If that doesn't succeed, it will go through all children and try
  // to set focus.
  // Returns false if no focusable child was found.
  bool EnsureFocus();

  // Sets the widget that should be focused when this window is activated next
  // time.
  // This should not be used to change focus. Call TBWidget::SetFocus to focus,
  // which will call this method if the window is inactive!
  void SetLastFocus(TBWidget* last_focus) { m_last_focus.Set(last_focus); }

  // Sets settings for how this window should look and behave.
  void SetSettings(WindowSettings settings);
  WindowSettings GetSettings() const { return m_settings; }

  // ResizeFit specifies how ResizeToFitContent should resize the window.
  enum class ResizeFit {
    kPreferred,        // Fit the preferred size of all content.
    kMinimal,          // Fit the minimal size of all content.
    kCurrentOrNeeded,  // Fit the minimal or maximum size only if needed. Will
                       // keep the new size as close as possible to the current
                       // size.
  };

  // Gets a suitable rect for the window based on the contents and the given
  // fit.
  Rect GetResizeToFitContentRect(ResizeFit fit = ResizeFit::kPreferred);

  // Resizes the window to fit the its content. This is the same as doing
  // SetRect(GetResizeToFitContentRect(fit)).
  void ResizeToFitContent(ResizeFit fit = ResizeFit::kPreferred);

  // Sets the window title.
  void SetText(const char* text) override { m_textfield.SetText(text); }
  using TBWidget::SetText;
  std::string GetText() override { return m_textfield.GetText(); }

  // Gets the height of the title bar (or 0 if the WindowSettings say this
  // window shouldn't have any title bar).
  int GetTitleHeight();

  Rect GetPaddingRect() override;
  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;

  bool OnEvent(const TBWidgetEvent& ev) override;
  void OnAdded() override;
  void OnRemove() override;
  void OnChildAdded(TBWidget* child) override;
  void OnResized(int old_w, int old_h) override;

 protected:
  Window* GetTopMostOtherWindow(bool only_activable_windows);
  void SetWindowActiveState(bool active);
  void Deactivate();

  TBMover m_mover;
  TBResizer m_resizer;
  TBTextField m_textfield;
  TBButton m_close_button;
  WindowSettings m_settings = WindowSettings::kDefault;
  TBWidgetSafePointer m_last_focus;
};

}  // namespace tb

#endif  // TB_WINDOW_H
