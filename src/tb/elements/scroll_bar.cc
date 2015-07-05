/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/scroll_bar.h"
#include "tb/parsing/element_inflater.h"
#include "tb/util/math.h"
#include "tb/util/metrics.h"

namespace tb {
namespace elements {

void ScrollBar::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(ScrollBar, Value::Type::kFloat, ElementZ::kTop);
}

ScrollBar::ScrollBar()
    : m_axis(Axis::kY)  // Make SetAxis below always succeed and set the skin
{
  SetAxis(Axis::kX);
  AddChild(&m_handle);
}

ScrollBar::~ScrollBar() { RemoveChild(&m_handle); }

void ScrollBar::OnInflate(const parsing::InflateInfo& info) {
  auto axis = tb::from_string(info.node->GetValueString("axis", "x"), Axis::kY);
  SetAxis(axis);
  SetGravity(axis == Axis::kX ? Gravity::kLeftRight : Gravity::kTopBottom);
  Element::OnInflate(info);
}

void ScrollBar::SetAxis(Axis axis) {
  if (axis == m_axis) return;
  m_axis = axis;
  if (axis == Axis::kX) {
    SetSkinBg(TBIDC("ScrollBarBgX"), InvokeInfo::kNoCallbacks);
    m_handle.SetSkinBg(TBIDC("ScrollBarFgX"), InvokeInfo::kNoCallbacks);
  } else {
    SetSkinBg(TBIDC("ScrollBarBgY"), InvokeInfo::kNoCallbacks);
    m_handle.SetSkinBg(TBIDC("ScrollBarFgY"), InvokeInfo::kNoCallbacks);
  }
  Invalidate();
}

void ScrollBar::SetLimits(double min, double max, double visible) {
  max = std::max(min, max);
  visible = std::max(visible, 0.0);
  if (min == m_min && max == m_max && m_visible == visible) {
    return;
  }
  m_min = min;
  m_max = max;
  m_visible = visible;
  SetValueDouble(m_value);

  // If we're currently dragging the scrollbar handle, convert the down point
  // to root and then back after the applying the new limit.
  // This prevents sudden jumps to unexpected positions when scrolling.
  if (captured_element == &m_handle) {
    m_handle.ConvertToRoot(pointer_down_element_x, pointer_down_element_y);
  }

  UpdateHandle();

  if (captured_element == &m_handle) {
    m_handle.ConvertFromRoot(pointer_down_element_x, pointer_down_element_y);
  }
}

void ScrollBar::SetValueDouble(double value) {
  value = util::Clamp(value, m_min, m_max);
  if (value == m_value) {
    return;
  }
  m_value = value;

  UpdateHandle();
  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);
}

bool ScrollBar::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kPointerMove && captured_element == &m_handle) {
    if (m_to_pixel_factor > 0) {
      int dx = ev.target_x - pointer_down_element_x;
      int dy = ev.target_y - pointer_down_element_y;
      double delta_val = (m_axis == Axis::kX ? dx : dy) / m_to_pixel_factor;
      SetValueDouble(m_value + delta_val);
    }
    return true;
  } else if (ev.type == EventType::kPointerMove && ev.target == this) {
    return true;
  } else if (ev.type == EventType::kPointerDown && ev.target == this) {
    bool after_handle = m_axis == Axis::kX ? ev.target_x > m_handle.rect().x
                                           : ev.target_y > m_handle.rect().y;
    SetValueDouble(m_value + (after_handle ? m_visible : -m_visible));
    return true;
  } else if (ev.type == EventType::kWheel) {
    double old_val = m_value;
    SetValueDouble(m_value + ev.delta_y * util::GetPixelsPerLine());
    return m_value != old_val;
  }
  return false;
}

void ScrollBar::UpdateHandle() {
  // Calculate the mover size and position.
  bool horizontal = m_axis == Axis::kX;
  int available_pixels = horizontal ? rect().w : rect().h;
  int min_thickness_pixels = std::min(rect().h, rect().w);

  int visible_pixels = available_pixels;

  if (m_max - m_min > 0 && m_visible > 0) {
    double visible_proportion = m_visible / (m_visible + m_max - m_min);
    visible_pixels = (int)(visible_proportion * available_pixels);

    // Limit the size of the indicator to the slider thickness so that it
    // doesn't become too tiny when the visible proportion is very small.
    visible_pixels = std::max(visible_pixels, min_thickness_pixels);

    m_to_pixel_factor =
        double(available_pixels - visible_pixels) / (m_max - m_min) /*+ 0.5*/;
  } else {
    m_to_pixel_factor = 0;

    // If we can't scroll anything, make the handle invisible.
    visible_pixels = 0;
  }

  int pixel_pos = (int)(m_value * m_to_pixel_factor);

  Rect handle_rect;
  if (horizontal) {
    handle_rect.reset(pixel_pos, 0, visible_pixels, rect().h);
  } else {
    handle_rect.reset(0, pixel_pos, rect().w, visible_pixels);
  }

  m_handle.set_rect(handle_rect);
}

void ScrollBar::OnResized(int old_w, int old_h) { UpdateHandle(); }

}  // namespace elements
}  // namespace tb
