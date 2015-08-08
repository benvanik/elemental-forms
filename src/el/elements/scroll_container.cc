/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/scroll_container.h"
#include "el/parsing/element_inflater.h"
#include "el/util/debug.h"
#include "el/util/metrics.h"

namespace el {
namespace elements {

// Helper for ScrollContainer or any other scrollable container that needs to
// solve scrollbar visibility according to ScrollMode.
class ScrollBarVisibility {
 public:
  ScrollBarVisibility() = default;

  static ScrollBarVisibility Solve(ScrollMode mode, int content_w,
                                   int content_h, int available_w,
                                   int available_h, int scrollbar_x_h,
                                   int scrollbar_y_w);
  static bool IsAlwaysOnX(ScrollMode mode) { return mode == ScrollMode::kXY; }
  static bool IsAlwaysOnY(ScrollMode mode) {
    return mode == ScrollMode::kXY || mode == ScrollMode::kY;
  }

 public:
  bool x_on = false;
  bool y_on = false;
  int visible_w = 0;
  int visible_h = 0;
};

ScrollBarVisibility ScrollBarVisibility::Solve(ScrollMode mode, int content_w,
                                               int content_h, int available_w,
                                               int available_h,
                                               int scrollbar_x_h,
                                               int scrollbar_y_w) {
  ScrollBarVisibility visibility;
  visibility.visible_w = available_w;
  visibility.visible_h = available_h;
  switch (mode) {
    case ScrollMode::kXY:
      // X and Y always; scroll-mode: xy
      visibility.y_on = true;
      visibility.x_on = true;
      visibility.visible_w -= scrollbar_y_w;
      visibility.visible_h -= scrollbar_x_h;
      break;
    case ScrollMode::kY:
      // Y always (X never); scroll-mode: y
      visibility.y_on = true;
      visibility.visible_w -= scrollbar_y_w;
      break;
    case ScrollMode::kAutoY:
      // Y auto (X never); scroll-mode: y-auto
      if (content_h > available_h) {
        visibility.y_on = true;
        visibility.visible_w -= scrollbar_y_w;
      }
      break;
    case ScrollMode::kAutoXAutoY:
      // X auto, Y auto; scroll-mode: auto
      if (content_w > visibility.visible_w) {
        visibility.x_on = true;
        visibility.visible_h = available_h - scrollbar_x_h;
      }
      if (content_h > visibility.visible_h) {
        visibility.y_on = true;
        visibility.visible_w = available_w - scrollbar_y_w;
      }
      if (content_w > visibility.visible_w) {
        visibility.x_on = true;
        visibility.visible_h = available_h - scrollbar_x_h;
      }
      break;
    case ScrollMode::kOff:
      // X and Y never; scroll-mode: off
      break;
  }
  return visibility;
}

void ScrollContainer::ScrollContainerRoot::OnPaintChildren(
    const PaintProps& paint_props) {
  // We only want clipping in one axis (the overflowing one) so we
  // don't damage any expanded skins on the other axis. Add some fluff.
  const int fluff = 100;
  ScrollContainer* sc = static_cast<ScrollContainer*>(parent());
  Rect clip_rect = padding_rect().Expand(
      sc->m_scrollbar_x.can_scroll_negative() ? 0 : fluff,
      sc->m_scrollbar_y.can_scroll_negative() ? 0 : fluff,
      sc->m_scrollbar_x.can_scroll_positive() ? 0 : fluff,
      sc->m_scrollbar_y.can_scroll_positive() ? 0 : fluff);

  Rect old_clip_rect =
      graphics::Renderer::get()->set_clip_rect(clip_rect, true);

  EL_IF_DEBUG_SETTING(
      util::DebugInfo::Setting::kLayoutClipping,
      graphics::Renderer::get()->DrawRect(clip_rect, Color(255, 0, 0, 200)));

  Element::OnPaintChildren(paint_props);

  graphics::Renderer::get()->set_clip_rect(old_clip_rect, false);
}

void ScrollContainer::ScrollContainerRoot::GetChildTranslation(int& x,
                                                               int& y) const {
  ScrollContainer* sc = static_cast<ScrollContainer*>(parent());
  x = -sc->m_scrollbar_x.value();
  y = -sc->m_scrollbar_y.value();
}

void ScrollContainer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(ScrollContainer, Value::Type::kNull,
                               ElementZ::kTop);
}

ScrollContainer::ScrollContainer() {
  AddChild(&m_scrollbar_x);
  AddChild(&m_scrollbar_y);
  AddChild(&m_root);
  m_scrollbar_y.set_axis(Axis::kY);
}

ScrollContainer::~ScrollContainer() {
  RemoveChild(&m_root);
  RemoveChild(&m_scrollbar_y);
  RemoveChild(&m_scrollbar_x);
}

void ScrollContainer::OnInflate(const parsing::InflateInfo& info) {
  set_gravity(Gravity::kAll);
  set_adapt_content_size(
      info.node->GetValueInt("adapt-content", is_adapting_content_size())
          ? true
          : false);
  set_adapt_to_content_size(
      info.node->GetValueInt("adapt-to-content", is_adapting_to_content_size())
          ? true
          : false);
  if (const char* mode = info.node->GetValueString("scroll-mode", nullptr)) {
    set_scroll_mode(from_string(mode, scroll_mode()));
  }
  Element::OnInflate(info);
}

void ScrollContainer::set_adapt_to_content_size(bool adapt) {
  if (m_adapt_to_content_size == adapt) return;
  InvalidateLayout(InvalidationMode::kRecursive);
  m_adapt_to_content_size = adapt;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void ScrollContainer::set_adapt_content_size(bool adapt) {
  if (m_adapt_content_size == adapt) return;
  m_adapt_content_size = adapt;
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void ScrollContainer::set_scroll_mode(ScrollMode mode) {
  if (mode == m_mode) return;
  m_mode = mode;
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void ScrollContainer::ScrollTo(int x, int y) {
  int old_x = m_scrollbar_x.value();
  int old_y = m_scrollbar_y.value();
  m_scrollbar_x.set_value(x);
  m_scrollbar_y.set_value(y);
  if (old_x != m_scrollbar_x.value() || old_y != m_scrollbar_y.value()) {
    Invalidate();
  }
}

Element::ScrollInfo ScrollContainer::scroll_info() {
  ScrollInfo info;
  info.min_x = static_cast<int>(m_scrollbar_x.min_value());
  info.min_y = static_cast<int>(m_scrollbar_y.min_value());
  info.max_x = static_cast<int>(m_scrollbar_x.max_value());
  info.max_y = static_cast<int>(m_scrollbar_y.max_value());
  info.x = m_scrollbar_x.value();
  info.y = m_scrollbar_y.value();
  return info;
}

void ScrollContainer::InvalidateLayout(InvalidationMode il) {
  m_layout_is_invalid = true;
  // No recursion up to parents here unless we adapt to content size.
  if (m_adapt_to_content_size) Element::InvalidateLayout(il);
}

Rect ScrollContainer::padding_rect() {
  int visible_w = rect().w;
  int visible_h = rect().h;
  if (m_scrollbar_x.opacity()) {
    visible_h -= m_scrollbar_x.GetPreferredSize().pref_h;
  }
  if (m_scrollbar_y.opacity()) {
    visible_w -= m_scrollbar_y.GetPreferredSize().pref_w;
  }
  return Rect(0, 0, visible_w, visible_h);
}

PreferredSize ScrollContainer::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps;
  ps.pref_w = ps.pref_h = 100;
  ps.min_w = ps.min_h = 50;
  if (m_adapt_to_content_size) {
    if (Element* content_child = m_root.first_child()) {
      ps = content_child->GetPreferredSize(constraints);
      int scrollbar_y_w = m_scrollbar_y.GetPreferredSize().pref_w;
      int scrollbar_x_h = m_scrollbar_x.GetPreferredSize().pref_h;

      ps.pref_w += scrollbar_y_w;
      ps.max_w += scrollbar_y_w;

      if (m_mode == ScrollMode::kXY || m_mode == ScrollMode::kAutoXAutoY) {
        ps.pref_h += scrollbar_x_h;
        ps.max_h += scrollbar_x_h;
      }
    }
  }
  return ps;
}

bool ScrollContainer::OnEvent(const Event& ev) {
  if (ev.type == EventType::kChanged &&
      (ev.target == &m_scrollbar_x || ev.target == &m_scrollbar_y)) {
    Invalidate();
    OnScroll(m_scrollbar_x.value(), m_scrollbar_y.value());
    return true;
  } else if (ev.type == EventType::kWheel &&
             ev.modifierkeys == ModifierKeys::kNone) {
    double old_val_y = m_scrollbar_y.double_value();
    m_scrollbar_y.set_double_value(old_val_y +
                                   ev.delta_y * util::GetPixelsPerLine());
    double old_val_x = m_scrollbar_x.double_value();
    m_scrollbar_x.set_double_value(old_val_x +
                                   ev.delta_x * util::GetPixelsPerLine());
    if (m_scrollbar_x.double_value() != old_val_x ||
        m_scrollbar_y.double_value() != old_val_y) {
      return true;
    }
  } else if (ev.type == EventType::kKeyDown) {
    if (ev.special_key == SpecialKey::kLeft &&
        m_scrollbar_x.can_scroll_negative()) {
      ScrollBySmooth(-util::GetPixelsPerLine(), 0);
    } else if (ev.special_key == SpecialKey::kRight &&
               m_scrollbar_x.can_scroll_positive()) {
      ScrollBySmooth(util::GetPixelsPerLine(), 0);
    } else if (ev.special_key == SpecialKey::kUp &&
               m_scrollbar_y.can_scroll_negative()) {
      ScrollBySmooth(0, -util::GetPixelsPerLine());
    } else if (ev.special_key == SpecialKey::kDown &&
               m_scrollbar_y.can_scroll_positive()) {
      ScrollBySmooth(0, util::GetPixelsPerLine());
    } else if (ev.special_key == SpecialKey::kPageUp &&
               m_scrollbar_y.can_scroll_negative()) {
      ScrollBySmooth(0, -padding_rect().h);
    } else if (ev.special_key == SpecialKey::kPageDown &&
               m_scrollbar_y.can_scroll_positive()) {
      ScrollBySmooth(0, padding_rect().h);
    } else if (ev.special_key == SpecialKey::kHome) {
      ScrollToSmooth(m_scrollbar_x.value(), 0);
    } else if (ev.special_key == SpecialKey::kEnd) {
      ScrollToSmooth(m_scrollbar_x.value(), (int)m_scrollbar_y.max_value());
    } else {
      return Element::OnEvent(ev);
    }
    return true;
  }
  return Element::OnEvent(ev);
}

void ScrollContainer::OnProcess() {
  SizeConstraints sc(rect().w, rect().h);
  ValidateLayout(sc);
}

void ScrollContainer::ValidateLayout(const SizeConstraints& constraints) {
  if (!m_layout_is_invalid) return;
  m_layout_is_invalid = false;

  // Layout scrollbars (no matter if they are visible or not).
  int scrollbar_y_w = m_scrollbar_y.GetPreferredSize().pref_w;
  int scrollbar_x_h = m_scrollbar_x.GetPreferredSize().pref_h;
  m_scrollbar_x.set_rect(
      {0, rect().h - scrollbar_x_h, rect().w - scrollbar_y_w, scrollbar_x_h});
  m_scrollbar_y.set_rect(
      {rect().w - scrollbar_y_w, 0, scrollbar_y_w, rect().h});

  if (Element* content_child = m_root.first_child()) {
    int horizontal_padding =
        ScrollBarVisibility::IsAlwaysOnY(m_mode) ? scrollbar_y_w : 0;
    int vertical_padding =
        ScrollBarVisibility::IsAlwaysOnX(m_mode) ? scrollbar_x_h : 0;

    SizeConstraints inner_sc =
        constraints.ConstrainByPadding(horizontal_padding, vertical_padding);

    PreferredSize ps = content_child->GetPreferredSize(inner_sc);

    auto visibility =
        ScrollBarVisibility::Solve(m_mode, ps.pref_w, ps.pref_h, rect().w,
                                   rect().h, scrollbar_x_h, scrollbar_y_w);
    m_scrollbar_x.set_opacity(visibility.x_on ? 1.f : 0.f);
    m_scrollbar_y.set_opacity(visibility.y_on ? 1.f : 0.f);
    m_root.set_rect({0, 0, visibility.visible_w, visibility.visible_h});

    int content_w, content_h;
    if (m_adapt_content_size) {
      content_w = std::max(ps.pref_w, m_root.rect().w);
      content_h = std::max(ps.pref_h, m_root.rect().h);
      if (!visibility.x_on && m_root.rect().w < ps.pref_w) {
        content_w = std::min(ps.pref_w, m_root.rect().w);
      }
    } else {
      content_w = ps.pref_w;
      content_h = ps.pref_h;
    }

    content_child->set_rect(Rect(0, 0, content_w, content_h));
    double limit_max_w = std::max(0, content_w - m_root.rect().w);
    double limit_max_h = std::max(0, content_h - m_root.rect().h);
    m_scrollbar_x.set_limits(0, limit_max_w, m_root.rect().w);
    m_scrollbar_y.set_limits(0, limit_max_h, m_root.rect().h);
  }
}

void ScrollContainer::OnResized(int old_w, int old_h) {
  InvalidateLayout(InvalidationMode::kTargetOnly);
  SizeConstraints sc(rect().w, rect().h);
  ValidateLayout(sc);
}

}  // namespace elements
}  // namespace el
