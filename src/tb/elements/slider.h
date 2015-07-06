/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_SLIDER_H_
#define TB_ELEMENTS_SLIDER_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// A horizontal or vertical slider for a number within a range.
// FIX: Add a "track value" showing as a line within the track (to be used for
// buffering etc).
// FIX: Also add a auto track that keeps it up to date with value (default).
class Slider : public Element {
 public:
  TBOBJECT_SUBCLASS(Slider, Element);
  static void RegisterInflater();

  Slider();
  ~Slider() override;

  Axis axis() const override { return m_axis; }
  // Sets along which axis the scrollbar should scroll.
  void set_axis(Axis axis) override;

  double min_value() const { return m_min; }
  double max_value() const { return m_max; }
  // Sets the min, max limits for the slider.
  void set_limits(double min, double max);

  // Gets a small value (depending on the min and max limits) for stepping by
  // f.ex. keyboard.
  double small_step() const { return (m_max - m_min) / 100.0; }

  double double_value() override { return m_value; }
  // Same as SetValue, but with double precision.
  void set_double_value(double value) override;

  int value() override { return (int)double_value(); }
  void set_value(int value) override { set_double_value(value); }

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const Event& ev) override;
  void OnResized(int old_w, int old_h) override;

 protected:
  Element m_handle;
  Axis m_axis;
  double m_value = 0;
  double m_min = 0;
  double m_max = 1;
  double m_to_pixel_factor = 0;
  void UpdateHandle();
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_SLIDER_H_
