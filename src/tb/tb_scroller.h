/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_SCROLLER_H
#define TB_SCROLLER_H

#include "tb/message_handler.h"

namespace tb {

class Element;

// Does the calculations of time, speed and distance that decides how the slow
// down of a scroll will happen.
// NOTE: Speed is in pixels per millisecond. Duration is in milliseconds and
// distance is in pixels. Distance and speed may be negative!
class ScrollerFunction {
 public:
  ScrollerFunction(float decay) : m_decay(decay) {}

  // Calculates the duration needed until the end distance is reached from the
  // given start speed.
  float GetDurationFromSpeed(float start_speed);

  // Calculates the start speed needed to reach the given distance.
  float GetSpeedFromDistance(float distance);

  // Calculates the distance reached at the given elapsed_time_ms with the given
  // start_speed.
  float GetDistanceAtTime(float start_speed, float elapsed_time_ms);

  // Same as GetDistanceAtTime but rounded to integer.
  int GetDistanceAtTimeInt(float start_speed, float elapsed_time_ms);

 private:
  float m_decay;
};

// May override the target scroll position of a Scroller.
class ScrollerSnapListener {
 public:
  virtual ~ScrollerSnapListener() = default;

  // Called when the target scroll position is calculated.
  // target_element is the element being scroller.
  // target_x, target_y is the suggested target scroll position which may be
  // changed to something else in this call.
  // NOTE: The scroll positions are relative to the target element (inner
  // scrolled Element). If there's nested scrollable elements, only the
  // inner scrolled element applies snapping.
  virtual void OnScrollSnap(Element* target_element, int& target_x,
                            int& target_y) = 0;
};

// Scroller handles panning while the pointer is down and measure the pan speed
// over time. It also handles continued scrolling when the pointer has been
// released with a flick.
class Scroller : private MessageHandler {
 public:
  Scroller(Element* target);
  ~Scroller() override;

  // Sets the listener that may override the target scroll position.
  void SetSnapListener(ScrollerSnapListener* listener) {
    m_snap_listener = listener;
  }

  // Starts tracking pan movement from calls to OnPan.
  void Start();

  // Stops tracking pan movement from calls to OnPan, or stop any ongoing
  // scrolling.
  void Stop();

  // Returns true if the pan tracking is started or.
  bool IsStarted() const { return m_is_started; }

  // Gets the element that will be panned/scrolled. Any parent of this element
  // may also be panned/scrolled.
  Element* GetTarget() const { return m_target; }

  // Pans the target element (or any parent) with the given deltas.
  // Should be called while the pointer is down. This will track the pan speed
  // over time.
  bool OnPan(int dx, int dy);

  // Called when the panning ends and the scroller should start scrolling.
  // Should be called when the pointer is released.
  void OnPanReleased();

  // Starts the scroller based on the given delta. Doesn't require previous
  // calls to OnPan or OnPanReleased.
  // If accumulative is true, the given delta will be added to any on going
  // scroll. If it's false, any ongoing scroll will be canceled.
  void OnScrollBy(int dx, int dy, bool accumulative);

 private:
  void OnMessageReceived(Message* msg) override;
  bool IsScrolling();
  bool StopIfAlmostStill();
  void StopOrSnapScroll();
  void Reset();
  void AdjustToSnappingAndScroll(float ppms_x, float ppms_y);
  void Scroll(float start_speed_ppms_x, float start_speed_ppms_y);
  void GetTargetChildTranslation(int& x, int& y) const;
  void GetTargetScrollXY(int& x, int& y) const;

  Element* m_target = nullptr;
  ScrollerSnapListener* m_snap_listener = nullptr;
  ScrollerFunction m_func;
  bool m_is_started = false;
  float m_pan_dx = 0, m_pan_dy = 0;
  float m_previous_pan_dx = 0;
  float m_previous_pan_dy = 0;
  double m_pan_time_ms = 0;
  double m_pan_delta_time_ms = 0;
  float m_scroll_start_speed_ppms_x = 0;
  float m_scroll_start_speed_ppms_y = 0;
  double m_scroll_start_ms = 0;
  float m_scroll_duration_x_ms = 0;
  float m_scroll_duration_y_ms = 0;
  int m_scroll_start_scroll_x = 0;
  int m_scroll_start_scroll_y = 0;
  float m_pan_power_multiplier_x = 1;
  float m_pan_power_multiplier_y = 1;
  int m_expected_scroll_x = 0;
  int m_expected_scroll_y = 0;
};

}  // namespace tb

#endif  // TB_SCROLLER_H
