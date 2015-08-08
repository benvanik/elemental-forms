/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_FORM_H_
#define EL_ELEMENTS_FORM_H_

#include <string>

#include "el/element_listener.h"
#include "el/elements/button.h"
#include "el/elements/label.h"
#include "el/elements/mover.h"
#include "el/elements/resizer.h"

namespace el {
namespace elements {

enum class FormSettings {
  kNone = 0,               // Chrome less form without any other settings.
  kTitleBar = 1 << 0,      // Show a title bar that can also move the form.
  kResizable = 1 << 1,     // Show an element for resizing the form.
  kCloseButton = 1 << 2,   // Show an element for closing the form.
  kCanActivate = 1 << 3,   // Can be activated and deactivate other forms.
  kDesignButton = 1 << 4,  // Show a button to open a designer for the form.
  kFullScreen = 1 << 5,    // Fully fill parent.

  kDefault =
      kTitleBar | kResizable | kCloseButton | kCanActivate | kDesignButton,
};
MAKE_ENUM_FLAG_COMBO(FormSettings);

// A Element that provides some form-like features.
// It can have a titlebar, be movable, resizable etc.
// It will activate and deactivate other forms on click (which will restore
// focus to the last focused child element).
class Form : public Element {
 public:
  TBOBJECT_SUBCLASS(Form, Element);

  Form();
  ~Form() override;

  // Closes this form.
  // NOTE: This form will be deleted after this call!
  void Close();

  // Returns true if this form is active.
  bool is_active() const;

  // Activates this form if it's not already activated.
  // This will deactivate any currently activated form.
  // This will automatically call EnsureFocus to restore/set focus to this
  // form.
  void Activate();

  // Ensures that this form has focus by attempting to find a focusable child
  // element.
  // It will first try to restore focus to the last focused element in this
  // form, or a element that has received SetFocus while the form was
  // inactive. If that doesn't succeed, it will go through all children and try
  // to set focus.
  // Returns false if no focusable child was found.
  bool EnsureFocus();

  // Sets the element that should be focused when this form is activated next
  // time.
  // This should not be used to change focus. Call Element::SetFocus to focus,
  // which will call this method if the form is inactive!
  void set_last_focus(Element* last_focus) {
    last_focus_element_.reset(last_focus);
  }

  FormSettings settings() const { return form_settings_; }
  // Sets settings for how this form should look and behave.
  void set_settings(FormSettings settings);

  void CenterInParent();

  // ResizeFit specifies how ResizeToFitContent should resize the form.
  enum class ResizeFit {
    kPreferred,        // Fit the preferred size of all content.
    kMinimal,          // Fit the minimal size of all content.
    kCurrentOrNeeded,  // Fit the minimal or maximum size only if needed. Will
                       // keep the new size as close as possible to the current
                       // size.
  };

  // Gets a suitable rect for the form based on the contents and the given
  // fit.
  Rect GetResizeToFitContentRect(ResizeFit fit = ResizeFit::kPreferred);

  // Resizes the form to fit the its content. This is the same as doing
  // set_rect(GetResizeToFitContentRect(fit)).
  void ResizeToFitContent(ResizeFit fit = ResizeFit::kPreferred);

  std::string text() override { return title_label_.text(); }
  // Sets the form title.
  void set_text(const char* text) override { title_label_.set_text(text); }
  using Element::set_text;

  // Gets the height of the title bar (or 0 if the FormSettings say this
  // form shouldn't have any title bar).
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
  Form* GetTopMostOtherForm(bool only_activable_forms);
  void SetFormActiveState(bool active);
  void Deactivate();

  elements::Mover title_mover_;
  elements::Resizer title_resizer_;
  elements::Label title_label_;
  elements::Button title_design_button_;
  elements::Button title_close_button_;
  FormSettings form_settings_ = FormSettings::kDefault;
  WeakElementPointer last_focus_element_;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_FORM_H_
