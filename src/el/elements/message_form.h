/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_MESSAGE_FORM_H_
#define EL_ELEMENTS_MESSAGE_FORM_H_

#include "el/element_listener.h"
#include "el/elements/form.h"

namespace el {
namespace elements {

enum class MessageFormButtons {
  kOk,
  kOkCancel,
  kYesNo,
};

// Contains additional settings for MessageForm.
class MessageFormSettings {
 public:
  MessageFormSettings() : msg(MessageFormButtons::kOk) {}
  MessageFormSettings(MessageFormButtons msg, TBID icon_skin)
      : msg(msg), icon_skin(icon_skin) {}

 public:
  MessageFormButtons msg;  // The type of response for the message.
  TBID icon_skin;          // The icon skin (0 for no icon)
  bool dimmer = false;   // Set to true to dim background elements by a Dimmer.
  bool styling = false;  // Enable styling in the textfield.
};

// A form for showing simple messages.
// Events invoked in this form will travel up through the target element.
// When the user click any of its buttons, it will invoke a click event
// (with the form ID), with the clicked buttons id as ref_id.
// Then it will delete itself.
// If the target element is deleted while this form is alive, the form will
// delete itself.
class MessageForm : public Form, private ElementListener {
 public:
  TBOBJECT_SUBCLASS(MessageForm, Form);

  MessageForm(Element* target, TBID id);
  ~MessageForm() override;

  bool Show(const std::string& title, const std::string& message,
            MessageFormSettings* settings = nullptr);

  Element* event_destination() override { return m_target.get(); }

  bool OnEvent(const Event& ev) override;
  void OnDie() override;

 private:
  void AddButton(TBID id, bool focused);
  void OnElementDelete(Element* element) override;
  bool OnElementDying(Element* element) override;
  WeakElementPointer m_dimmer;
  WeakElementPointer m_target;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_MESSAGE_FORM_H_
