/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_WINDOW_H_
#define EL_WINDOW_H_

#include "el/element_listener.h"
#include "el/elements/button.h"
#include "el/elements/label.h"
#include "el/elements/mover.h"
#include "el/elements/resizer.h"

namespace el {

enum class WindowSettings {
  kNone = 0,               // Chrome less window without any other settings.
  kTitleBar = 1 << 0,      // Show a title bar that can also move the window.
  kResizable = 1 << 1,     // Show an element for resizing the window.
  kCloseButton = 1 << 2,   // Show an element for closing the window.
  kCanActivate = 1 << 3,   // Can be activated and deactivate other windows.
  kDesignButton = 1 << 4,  // Show a button to open a designer for the window.
  kFullScreen = 1 << 5,    // Fully fill parent.

  kDefault =
      kTitleBar | kResizable | kCloseButton | kCanActivate | kDesignButton,
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
  bool is_active() const;

  // Activates this window if it's not already activated.
  // This will deactivate any currently activated window.
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
  void set_last_focus(Element* last_focus) {
    last_focus_element_.reset(last_focus);
  }

  WindowSettings settings() const { return window_settings_; }
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

  std::string text() override { return title_label_.text(); }
  // Sets the window title.
  void set_text(const char* text) override { title_label_.set_text(text); }
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

  void OpenDesigner();

 protected:
  Window* GetTopMostOtherWindow(bool only_activable_windows);
  void SetWindowActiveState(bool active);
  void Deactivate();

  elements::Mover title_mover_;
  elements::Resizer title_resizer_;
  elements::Label title_label_;
  elements::Button title_design_button_;
  elements::Button title_close_button_;
  WindowSettings window_settings_ = WindowSettings::kDefault;
  WeakElementPointer last_focus_element_;
};

}  // namespace el

#endif  // EL_WINDOW_H_
