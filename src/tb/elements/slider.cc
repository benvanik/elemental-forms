/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Seger�s and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/slider.h"
#include "tb/parsing/element_inflater.h"
#include "tb/util/math.h"

namespace tb {
namespace elements {

void Slider::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Slider, Value::Type::kFloat, ElementZ::kTop);
}

Slider::Slider()
    : m_axis(Axis::kY)  // Make SetAxis below always succeed and set the skin
{
  SetIsFocusable(true);
  SetAxis(Axis::kX);
  AddChild(&m_handle);
}

Slider::~Slider() { RemoveChild(&m_handle); }

void Slider::OnInflate(const parsing::InflateInfo& info) {
  auto axis = tb::from_string(info.node->GetValueString("axis", "x"), Axis::kY);
  SetAxis(axis);
  SetGravity(axis == Axis::kX ? Gravity::kLeftRight : Gravity::kTopBottom);
  double min = double(info.node->GetValueFloat("min", (float)GetMinValue()));
  double max = double(info.node->GetValueFloat("max", (float)GetMaxValue()));
  SetLimits(min, max);
  Element::OnInflate(info);
}

void Slider::SetAxis(Axis axis) {
  if (axis == m_axis) return;
  m_axis = axis;
  if (axis == Axis::kX) {
    SetSkinBg(TBIDC("SliderBgX"), InvokeInfo::kNoCallbacks);
    m_handle.SetSkinBg(TBIDC("SliderFgX"), InvokeInfo::kNoCallbacks);
  } else {
    SetSkinBg(TBIDC("SliderBgY"), InvokeInfo::kNoCallbacks);
    m_handle.SetSkinBg(TBIDC("SliderFgY"), InvokeInfo::kNoCallbacks);
  }
  Invalidate();
}

void Slider::SetLimits(double min, double max) {
  min = std::min(min, max);
  if (min == m_min && max == m_max) {
    return;
  }
  m_min = min;
  m_max = max;
  SetValueDouble(m_value);
  UpdateHandle();
}

void Slider::SetValueDouble(double value) {
  value = util::Clamp(value, m_min, m_max);
  if (value == m_value) return;
  m_value = value;

  UpdateHandle();
  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);
}

bool Slider::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kPointerMove && captured_element == &m_handle) {
    if (m_to_pixel_factor > 0) {
      int dx = ev.target_x - pointer_down_element_x;
      int dy = ev.target_y - pointer_down_element_y;
      double delta_val = (m_axis == Axis::kX ? dx : -dy) / m_to_pixel_factor;
      SetValueDouble(m_value + delta_val);
    }
    return true;
  } else if (ev.type == EventType::kWheel) {
    double old_val = m_value;
    double step = (m_axis == Axis::kX ? GetSmallStep() : -GetSmallStep());
    SetValueDouble(m_value + step * ev.delta_y);
    return m_value != old_val;
  } else if (ev.type == EventType::kKeyDown) {
    double step = (m_axis == Axis::kX ? GetSmallStep() : -GetSmallStep());
    if (ev.special_key == SpecialKey::kLeft ||
        ev.special_key == SpecialKey::kUp) {
      SetValueDouble(GetValueDouble() - step);
    } else if (ev.special_key == SpecialKey::kRight ||
               ev.special_key == SpecialKey::kDown) {
      SetValueDouble(GetValueDouble() + step);
    } else {
      return false;
    }
    return true;
  } else if (ev.type == EventType::kKeyUp) {
    if (ev.special_key == SpecialKey::kLeft ||
        ev.special_key == SpecialKey::kUp ||
        ev.special_key == SpecialKey::kRight ||
        ev.special_key == SpecialKey::kDown) {
      return true;
    }
  }
  return false;
}

void Slider::UpdateHandle() {
  // Calculate the handle position.
  bool horizontal = m_axis == Axis::kX;
  int available_pixels = horizontal ? rect().w : rect().h;

  Rect handle_rect;
  if (m_max - m_min > 0) {
    PreferredSize ps = m_handle.GetPreferredSize();
    int handle_pixels = horizontal ? ps.pref_w : ps.pref_h;
    m_to_pixel_factor =
        double(available_pixels - handle_pixels) / (m_max - m_min) /*+ 0.5*/;

    int pixel_pos = int((m_value - m_min) * m_to_pixel_factor);

    if (horizontal) {
      handle_rect.reset(pixel_pos, (rect().h - ps.pref_h) / 2, ps.pref_w,
                        ps.pref_h);
    } else {
      handle_rect.reset((rect().w - ps.pref_w) / 2,
                        rect().h - handle_pixels - pixel_pos, ps.pref_w,
                        ps.pref_h);
    }
  } else {
    m_to_pixel_factor = 0;
  }

  m_handle.set_rect(handle_rect);
}

void Slider::OnResized(int old_w, int old_h) { UpdateHandle(); }

}  // namespace elements
}  // namespace tb