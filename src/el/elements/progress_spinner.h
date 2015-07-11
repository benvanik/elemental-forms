/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_PROGRESS_SPINNER_H_
#define EL_ELEMENTS_PROGRESS_SPINNER_H_

#include "el/element.h"
#include "el/message_handler.h"

namespace el {
namespace elements {

// An animation that is running while its value is 1.
// Typically used to indicate that the application is working.
class ProgressSpinner : public Element, protected MessageHandler {
 public:
  TBOBJECT_SUBCLASS(ProgressSpinner, Element);
  static void RegisterInflater();

  ProgressSpinner();

  // Returns true if the animation is running.
  bool is_animating() { return m_value > 0; }

  // Begin/End are used to start or stop the animation in a incremental way.
  // If several tasks may activate the same spinner, calling Begin/End instead
  // of using SetValue, so it will keep running as long as any source wants it
  // to.
  void PushAnimating() { set_value(value() + 1); }
  void PopAnimating() { set_value(value() - 1); }

  int value() override { return m_value; }
  // Setting the value to 1 will start the spinner. Setting it to 0 will stop
  // it.
  void set_value(int value) override;

  void OnPaint(const PaintProps& paint_props) override;

  void OnMessageReceived(Message* msg) override;

 protected:
  // How fast should the spinner animation animate.
  // FIX: Add spin_speed to skin!
  // FIX: Make it post messages only if visible
  static const int kSpinSpeed = 1000 / 30;

  int m_value = 0;
  int m_frame = 0;
  TBID m_skin_fg;
};

}  // namespace elements
namespace dsl {

struct ProgressSpinnerNode : public ElementNode<ProgressSpinnerNode> {
  using R = ProgressSpinnerNode;
  ProgressSpinnerNode() : ElementNode("ProgressSpinner") {}
  //
  R& value(int32_t value) {
    set("value", value);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_PROGRESS_SPINNER_H_
