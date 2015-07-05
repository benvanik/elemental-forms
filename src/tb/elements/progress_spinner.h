/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_PROGRESS_SPINNER_H_
#define TB_ELEMENTS_PROGRESS_SPINNER_H_

#include "tb/element.h"
#include "tb/message_handler.h"

namespace tb {
namespace elements {

// An animation that is running while its value is 1.
// Typically used to indicate that the application is working.
class ProgressSpinner : public Element, protected MessageHandler {
 public:
  TBOBJECT_SUBCLASS(ProgressSpinner, Element);
  static void RegisterInflater();

  ProgressSpinner();

  // Returns true if the animation is running.
  bool IsRunning() { return m_value > 0; }

  // Begin/End are used to start or stop the animation in a incremental way.
  // If several tasks may activate the same spinner, calling Begin/End instead
  // of using SetValue, so it will keep running as long as any source wants it
  // to.
  void Begin() { SetValue(GetValue() + 1); }
  void End() { SetValue(GetValue() - 1); }

  // Setting the value to 1 will start the spinner. Setting it to 0 will stop
  // it.
  void SetValue(int value) override;
  int GetValue() override { return m_value; }

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
}  // namespace tb

#endif  // TB_ELEMENTS_PROGRESS_SPINNER_H_
