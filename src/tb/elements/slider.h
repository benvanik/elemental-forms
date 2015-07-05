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

  // Sets along which axis the scrollbar should scroll.
  void SetAxis(Axis axis) override;
  Axis GetAxis() const override { return m_axis; }

  // Sets the min, max limits for the slider.
  void SetLimits(double min, double max);

  double GetMinValue() const { return m_min; }
  double GetMaxValue() const { return m_max; }

  // Gets a small value (depending on the min and max limits) for stepping by
  // f.ex. keyboard.
  double GetSmallStep() const { return (m_max - m_min) / 100.0; }

  // Same as SetValue, but with double precision.
  void SetValueDouble(double value) override;
  double GetValueDouble() override { return m_value; }

  void SetValue(int value) override { SetValueDouble(value); }
  int GetValue() override { return (int)GetValueDouble(); }

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
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
