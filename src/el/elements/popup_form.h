/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_POPUP_FORM_H_
#define EL_ELEMENTS_POPUP_FORM_H_

#include "el/element_listener.h"
#include "el/elements/form.h"

namespace el {
namespace elements {

// Describes the preferred alignment of a popup relative to a target element or
// a given point.
// It calculates the rect to be used to match these preferences for any given
// popup and target.
class PopupAlignment {
 public:
  static const int kUnspecified = el::util::kInvalidDimension;

  // Aligns relative to the target element.
  PopupAlignment(Align align = Align::kBottom)
      : pos_in_root(kUnspecified, kUnspecified),
        align(align),
        expand_to_target_width(true) {}

  // Aligns relative to the given position (coordinates relative to the root
  // element).
  PopupAlignment(const Point& pos_in_root, Align align = Align::kBottom)
      : pos_in_root(pos_in_root), align(align), expand_to_target_width(true) {}

  // Aligns relative to the given position (coordinates relative to the root
  // element). Applies an additional offset.
  PopupAlignment(const Point& pos_in_root, const Point& pos_offset)
      : pos_in_root(pos_in_root),
        pos_offset(pos_offset),
        align(Align::kBottom),
        expand_to_target_width(true) {}

  // Calculates a good rect for the given popup form using its preferred size
  // and the preferred alignment information stored in this class.
  Rect GetAlignedRect(Element* popup, Element* target) const;

  Point pos_in_root;
  Point pos_offset;

  Align align;
  // If true the width of the popup will be at least the same as the target
  // element if the alignment is Align::kTop or Align::kBottom.
  bool expand_to_target_width;
};

// A popup form that redirects any child elements events through the given
// target. It will automatically close on click events that are not sent through
// this popup.
class PopupForm : public Form, private ElementListener {
 public:
  TBOBJECT_SUBCLASS(PopupForm, Form);

  PopupForm(Element* target);
  ~PopupForm() override;

  void Show(const PopupAlignment& alignment);

  Element* event_destination() override { return m_target.get(); }

  bool OnEvent(const Event& ev) override;

  const WeakElementPointer& target_element() { return m_target; }

 private:
  void OnElementFocusChanged(Element* element, bool focused) override;
  bool OnElementInvokeEvent(Element* element, const Event& ev) override;
  void OnElementDelete(Element* element) override;
  bool OnElementDying(Element* element) override;

  WeakElementPointer m_target;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_POPUP_FORM_H_
