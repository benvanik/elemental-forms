/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_WINDOW_H_
#define TB_WINDOW_H_

#include "tb/element_listener.h"
#include "tb/elements/button.h"
#include "tb/elements/label.h"
#include "tb/elements/mover.h"
#include "tb/elements/resizer.h"

namespace tb {

enum class WindowSettings {
  kNone = 0,         // Chrome less window without any other settings.
  kTitleBar = 1,     // Show a title bar that can also move the window.
  kResizable = 2,    // Show a element for resizing the window.
  kCloseButton = 4,  // Show a element for closing the window.
  kCanActivate = 8,  // Can be activated and deactivate other windows.

  kDefault = kTitleBar | kResizable | kCloseButton | kCanActivate,
};
MAKE_ENUM_FLAG_COMBO(WindowSettings);

// A Element that provides some window-like features.
// It can have a titlebar, be movable, resizable etc.
// It will activate and deactivate other windows on click (which will restore
// focus to the last focused child element).
class Window : public Element {
 public:
  TBOBJECT_SUBCLASS(Window, Element);

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
  // element.
  // It will first try to restore focus to the last focused element in this
  // window, or a element that has received SetFocus while the window was
  // inactive. If that doesn't succeed, it will go through all children and try
  // to set focus.
  // Returns false if no focusable child was found.
  bool EnsureFocus();

  // Sets the element that should be focused when this window is activated next
  // time.
  // This should not be used to change focus. Call Element::SetFocus to focus,
  // which will call this method if the window is inactive!
  void set_last_focus(Element* last_focus) { m_last_focus.reset(last_focus); }

  WindowSettings settings() const { return m_settings; }
  // Sets settings for how this window should look and behave.
  void set_settings(WindowSettings settings);

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
  // set_rect(GetResizeToFitContentRect(fit)).
  void ResizeToFitContent(ResizeFit fit = ResizeFit::kPreferred);

  std::string text() override { return m_textfield.text(); }
  // Sets the window title.
  void set_text(const char* text) override { m_textfield.set_text(text); }
  using Element::set_text;

  // Gets the height of the title bar (or 0 if the WindowSettings say this
  // window shouldn't have any title bar).
  int title_bar_height();

  Rect padding_rect() override;
  PreferredSize OnCalculatePreferredSize(
      const SizeConstraints& constraints) override;

  bool OnEvent(const Event& ev) override;
  void OnAdded() override;
  void OnRemove() override;
  void OnChildAdded(Element* child) override;
  void OnResized(int old_w, int old_h) override;

 protected:
  Window* GetTopMostOtherWindow(bool only_activable_windows);
  void SetWindowActiveState(bool active);
  void Deactivate();

  elements::Mover m_mover;
  elements::Resizer m_resizer;
  elements::Label m_textfield;
  elements::Button m_close_button;
  WindowSettings m_settings = WindowSettings::kDefault;
  WeakElementPointer m_last_focus;
};

}  // namespace tb

#endif  // TB_WINDOW_H_
