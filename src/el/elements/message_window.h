/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_MESSAGE_WINDOW_H_
#define EL_ELEMENTS_MESSAGE_WINDOW_H_

#include "el/element_listener.h"
#include "el/window.h"

namespace el {
namespace elements {

enum class MessageWindowButtons {
  kOk,
  kOkCancel,
  kYesNo,
};

// Contains additional settings for MessageWindow.
class MessageWindowSettings {
 public:
  MessageWindowSettings() : msg(MessageWindowButtons::kOk) {}
  MessageWindowSettings(MessageWindowButtons msg, TBID icon_skin)
      : msg(msg), icon_skin(icon_skin) {}

 public:
  MessageWindowButtons msg;  // The type of response for the message.
  TBID icon_skin;            // The icon skin (0 for no icon)
  bool dimmer = false;   // Set to true to dim background elements by a Dimmer.
  bool styling = false;  // Enable styling in the textfield.
};

// A window for showing simple messages.
// Events invoked in this window will travel up through the target element.
// When the user click any of its buttons, it will invoke a click event
// (with the window ID), with the clicked buttons id as ref_id.
// Then it will delete itself.
// If the target element is deleted while this window is alive, the window will
// delete itself.
class MessageWindow : public Window, private ElementListener {
 public:
  TBOBJECT_SUBCLASS(MessageWindow, Window);

  MessageWindow(Element* target, TBID id);
  ~MessageWindow() override;

  bool Show(const std::string& title, const std::string& message,
            MessageWindowSettings* settings = nullptr);

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

#endif  // EL_ELEMENTS_MESSAGE_WINDOW_H_
