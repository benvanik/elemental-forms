/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_SCROLL_BAR_H_
#define TB_ELEMENTS_SCROLL_BAR_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// A scroll bar in the given axis.
class ScrollBar : public Element {
 public:
  TBOBJECT_SUBCLASS(ScrollBar, Element);
  static void RegisterInflater();

  ScrollBar();
  ~ScrollBar() override;

  // Sets along which axis the scrollbar should scroll.
  void SetAxis(Axis axis) override;
  Axis GetAxis() const override { return m_axis; }

  // Sets the min, max limits for the scrollbar.
  // The visible parameter is how much of the range that is visible.
  // When this is called, the scrollbar might change value and invoke if the
  // current value is outside of the new limits.
  void SetLimits(double min, double max, double visible);

  // Returns true if the scrollbar has anywhere to go with the current limits.
  bool CanScroll() const { return m_visible > 0; }

  // Returns true if the scrollbar can scroll in the positive direction with its
  // current limits.
  bool CanScrollPositive() const { return m_value < m_max; }

  // Returns true if the scrollbar can scroll in the negative direction with its
  // current limits.
  bool CanScrollNegative() const { return m_value > m_min; }

  double GetMinValue() const { return m_min; }
  double GetMaxValue() const { return m_max; }
  double GetVisible() const { return m_visible; }

  // Same as SetValue, but with double precision.
  void SetValueDouble(double value) override;
  double GetValueDouble() override { return m_value; }

  void SetValue(int value) override { SetValueDouble(value); }
  int GetValue() override { return (int)GetValueDouble(); }

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
  void OnResized(int old_w, int old_h) override;

 protected:
  void UpdateHandle();

  Element m_handle;
  Axis m_axis;
  double m_value = 0;
  double m_min = 0;
  double m_max = 1;
  double m_visible = 1;
  double m_to_pixel_factor = 0;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_SCROLL_BAR_H_
