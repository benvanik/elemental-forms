/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_MODAL_FORM_H_
#define EL_ELEMENTS_MODAL_FORM_H_

#include <functional>

#include "el/element_listener.h"
#include "el/elements/form.h"

namespace el {
namespace elements {

// A form for showing modal messages.
// Events invoked in this form will travel up through the target element.
// When the user click any of its buttons, it will invoke a click event
// (with the form ID), with the clicked buttons id as ref_id.
// Then it will delete itself.
// If the target element is deleted while this form is alive, the form will
// delete itself.
class ModalForm : public Form, private ElementListener {
 public:
  TBOBJECT_SUBCLASS(ModalForm, Form);

  ModalForm(std::function<void()> on_close_callback);
  ~ModalForm() override;

  void Show(el::Element* root_element);

  static bool is_any_visible() { return visible_count_ > 0; }

 protected:
  static int visible_count_;

  virtual void BuildUI() = 0;

  bool OnEvent(const el::Event& ev) override;
  void OnDie() override;
  void OnRemove() override;
  void OnElementAdded(Element* parent, Element* element) override;
  bool OnElementDying(Element* element) override;

  std::function<void()> on_close_callback_;
  WeakElementPointer dimmer_;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_MODAL_FORM_H_
