/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/layout_box.h"
#include "el/parsing/element_inflater.h"
#include "el/skin.h"
#include "el/util/debug.h"
#include "el/util/metrics.h"

namespace el {
namespace elements {

using graphics::Renderer;

void LayoutBox::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(LayoutBox, Value::Type::kNull, ElementZ::kTop);
}

LayoutBox::LayoutBox(Axis axis) : m_axis(axis) {
  m_packed.layout_mode_size = uint32_t(LayoutSize::kGravity);
  m_packed.layout_mode_pos = uint32_t(LayoutPosition::kCenter);
  m_packed.layout_mode_overflow = uint32_t(LayoutOverflow::kClip);
  m_packed.layout_mode_dist = uint32_t(LayoutDistribution::kPreferred);
  m_packed.layout_mode_dist_pos = uint32_t(LayoutDistributionPosition::kCenter);
  m_packed.paint_overflow_fadeout = 1;
}

void LayoutBox::OnInflate(const parsing::InflateInfo& info) {
  if (const char* spacing = info.node->GetValueString("spacing", nullptr)) {
    set_spacing(Skin::get()->dimension_converter()->GetPxFromString(
        spacing, kSpacingFromSkin));
  }
  set_gravity(Gravity::kAll);
  if (const char* size = info.node->GetValueString("size", nullptr)) {
    set_layout_size(from_string(size, LayoutSize::kPreferred));
  }
  if (const char* pos = info.node->GetValueString("position", nullptr)) {
    LayoutPosition lp = LayoutPosition::kCenter;
    if (strstr(pos, "left") || strstr(pos, "top")) {
      lp = LayoutPosition::kLeftTop;
    } else if (strstr(pos, "right") || strstr(pos, "bottom")) {
      lp = LayoutPosition::kRightBottom;
    } else if (strstr(pos, "gravity")) {
      lp = LayoutPosition::kGravity;
    }
    set_layout_position(lp);
  }
  if (const char* pos = info.node->GetValueString("overflow", nullptr)) {
    set_layout_overflow(from_string(pos, LayoutOverflow::kClip));
  }
  if (const char* dist = info.node->GetValueString("distribution", nullptr)) {
    set_layout_distribution(from_string(dist, LayoutDistribution::kPreferred));
  }
  if (const char* dist =
          info.node->GetValueString("distribution-position", nullptr)) {
    LayoutDistributionPosition ld = LayoutDistributionPosition::kCenter;
    if (strstr(dist, "left") || strstr(dist, "top")) {
      ld = LayoutDistributionPosition::kLeftTop;
    } else if (strstr(dist, "right") || strstr(dist, "bottom")) {
      ld = LayoutDistributionPosition::kRightBottom;
    }
    set_layout_distribution_position(ld);
  }
  Element::OnInflate(info);
}

void LayoutBox::set_axis(Axis axis) {
  if (axis == m_axis) return;
  m_axis = axis;
  InvalidateLayout(InvalidationMode::kRecursive);
  InvalidateSkinStates();
}

void LayoutBox::set_spacing(int spacing) {
  if (spacing == m_spacing) return;
  m_spacing = spacing;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void LayoutBox::set_overflow_scroll(int overflow_scroll) {
  overflow_scroll = std::min(overflow_scroll, m_overflow);
  overflow_scroll = std::max(overflow_scroll, 0);
  if (overflow_scroll == m_overflow_scroll) {
    return;
  }
  m_overflow_scroll = overflow_scroll;
  Invalidate();
  if (m_axis == Axis::kX) {
    OnScroll(m_overflow_scroll, 0);
  } else {
    OnScroll(0, m_overflow_scroll);
  }
}

void LayoutBox::set_layout_size(LayoutSize size) {
  if (uint32_t(size) == m_packed.layout_mode_size) return;
  m_packed.layout_mode_size = uint32_t(size);
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void LayoutBox::set_layout_position(LayoutPosition pos) {
  if (uint32_t(pos) == m_packed.layout_mode_pos) return;
  m_packed.layout_mode_pos = uint32_t(pos);
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void LayoutBox::set_layout_overflow(LayoutOverflow overflow) {
  if (uint32_t(overflow) == m_packed.layout_mode_overflow) return;
  m_packed.layout_mode_overflow = uint32_t(overflow);
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void LayoutBox::set_layout_distribution(LayoutDistribution distribution) {
  if (uint32_t(distribution) == m_packed.layout_mode_dist) return;
  m_packed.layout_mode_dist = uint32_t(distribution);
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void LayoutBox::set_layout_distribution_position(
    LayoutDistributionPosition distribution_pos) {
  if (uint32_t(distribution_pos) == m_packed.layout_mode_dist_pos) return;
  m_packed.layout_mode_dist_pos = uint32_t(distribution_pos);
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void LayoutBox::set_layout_order(LayoutOrder order) {
  bool reversed = (order == LayoutOrder::kTopToBottom);
  if (reversed == m_packed.mode_reverse_order) return;
  m_packed.mode_reverse_order = reversed;
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void LayoutBox::InvalidateLayout(InvalidationMode il) {
  m_packed.layout_is_invalid = 1;
  // Continue invalidating parents (depending on il).
  Element::InvalidateLayout(il);
}

PreferredSize GetRotatedPreferredSize(const PreferredSize& ps, Axis axis) {
  if (axis == Axis::kX) return ps;
  PreferredSize psr;
  psr.max_w = ps.max_h;
  psr.max_h = ps.max_w;
  psr.min_w = ps.min_h;
  psr.min_h = ps.min_w;
  psr.pref_w = ps.pref_h;
  psr.pref_h = ps.pref_w;
  psr.size_dependency =
      (any(ps.size_dependency & SizeDependency::kWidthOnHeight)
           ? SizeDependency::kHeightOnWidth
           : SizeDependency::kNone) |
      (any(ps.size_dependency & SizeDependency::kHeightOnWidth)
           ? SizeDependency::kWidthOnHeight
           : SizeDependency::kNone);
  return psr;
}

SizeConstraints GetRotatedSizeConstraints(const SizeConstraints& sc,
                                          Axis axis) {
  return axis == Axis::kX ? sc
                          : SizeConstraints(sc.available_h, sc.available_w);
}

Rect GetRotatedRect(const Rect& rect, Axis axis) {
  if (axis == Axis::kX) return rect;
  return Rect(rect.y, rect.x, rect.h, rect.w);
}

Gravity GetRotatedGravity(Gravity gravity, Axis axis) {
  if (axis == Axis::kX) return gravity;
  Gravity r = Gravity::kNone;
  r |= any(gravity & Gravity::kLeft) ? Gravity::kTop : Gravity::kNone;
  r |= any(gravity & Gravity::kTop) ? Gravity::kLeft : Gravity::kNone;
  r |= any(gravity & Gravity::kRight) ? Gravity::kBottom : Gravity::kNone;
  r |= any(gravity & Gravity::kBottom) ? Gravity::kRight : Gravity::kNone;
  return r;
}

bool LayoutBox::QualifyForExpansion(Gravity gravity) const {
  if (m_packed.layout_mode_dist == uint32_t(LayoutDistribution::kAvailable)) {
    return true;
  }
  if (m_packed.layout_mode_dist == uint32_t(LayoutDistribution::kGravity) &&
      (any(gravity & Gravity::kLeft) && any(gravity & Gravity::kRight))) {
    return true;
  }
  return false;
}

int LayoutBox::GetWantedHeight(Gravity gravity, const PreferredSize& ps,
                               int available_height) const {
  int height = 0;
  switch (LayoutSize(m_packed.layout_mode_size)) {
    case LayoutSize::kGravity:
      height = (any(gravity & Gravity::kTop) && any(gravity & Gravity::kBottom))
                   ? available_height
                   : std::min(available_height, ps.pref_h);
      break;
    case LayoutSize::kPreferred:
      height = std::min(available_height, ps.pref_h);
      break;
    case LayoutSize::kAvailable:
      height = std::min(available_height, ps.max_h);
      break;
  };
  height = std::min(height, ps.max_h);
  return height;
}

Element* LayoutBox::GetNextNonCollapsedElement(Element* child) const {
  Element* next = GetNextInLayoutOrder(child);
  while (next && next->visibility() == Visibility::kGone) {
    next = GetNextInLayoutOrder(next);
  }
  return next;
}

int LayoutBox::GetTrailingSpace(Element* child, int spacing) const {
  if (spacing == 0) return 0;
  if (!GetNextNonCollapsedElement(child)) return 0;
  return spacing;
}

int LayoutBox::CalculateSpacing() {
  // Get spacing from skin, if not specified.
  int spacing = m_spacing;
  if (spacing == kSpacingFromSkin) {
    if (auto el = background_skin_element()) {
      spacing = el->spacing();
    }
    assert(kSpacingFromSkin == kSkinValueNotSpecified);
    if (spacing == kSpacingFromSkin /*|| spacing == kSkinValueNotSpecified*/) {
      spacing = Skin::get()->default_spacing();
    }
  }
  return spacing;
}

Element* LayoutBox::GetFirstInLayoutOrder() const {
  return m_packed.mode_reverse_order ? last_child() : first_child();
}

Element* LayoutBox::GetNextInLayoutOrder(Element* child) const {
  return m_packed.mode_reverse_order ? child->GetPrev() : child->GetNext();
}

void LayoutBox::ValidateLayout(const SizeConstraints& constraints,
                               PreferredSize* calculate_ps) {
  // Layout notes:
  // - All layout code is written for Axis::kX layout.
  //   Instead of duplicating the layout code for both Axis::kX and Axis::kY, we
  //   simply rotate the in data (rect, gravity, preferred size) and the outdata
  //   (rect).

  if (!calculate_ps) {
    if (!m_packed.layout_is_invalid) return;
    m_packed.layout_is_invalid = 0;
  } else {
    // Maximum size will grow below depending of the childrens maximum size.
    calculate_ps->max_w = calculate_ps->max_h = 0;
  }

  const int spacing = CalculateSpacing();
  const Rect padding_rect = this->padding_rect();
  const Rect layout_rect = GetRotatedRect(padding_rect, m_axis);

  auto inner_sc = constraints.ConstrainByPadding(rect().w - padding_rect.w,
                                                 rect().h - padding_rect.h);

  // Calculate totals for minimum and preferred width that we need for layout.
  int total_preferred_w = 0;
  int total_min_pref_diff_w = 0;
  int total_max_pref_diff_w = 0;
  for (Element* child = GetFirstInLayoutOrder(); child;
       child = GetNextInLayoutOrder(child)) {
    if (child->visibility() == Visibility::kGone) {
      continue;
    }

    const int ending_space = GetTrailingSpace(child, spacing);
    const PreferredSize ps =
        GetRotatedPreferredSize(child->GetPreferredSize(inner_sc), m_axis);
    const Gravity gravity = GetRotatedGravity(child->gravity(), m_axis);

    total_preferred_w += ps.pref_w + ending_space;
    total_min_pref_diff_w += ps.pref_w - ps.min_w;

    if (QualifyForExpansion(gravity)) {
      int capped_max_w = std::min(layout_rect.w, ps.max_w);
      total_max_pref_diff_w += capped_max_w - ps.pref_w;
    }

    if (calculate_ps) {
      calculate_ps->min_h = std::max(calculate_ps->min_h, ps.min_h);
      calculate_ps->pref_h = std::max(calculate_ps->pref_h, ps.pref_h);
      calculate_ps->min_w += ps.min_w + ending_space;
      calculate_ps->pref_w += ps.pref_w + ending_space;
      calculate_ps->max_w += ps.max_w + ending_space;

      // The element height depends on layout and element properties, so get
      // what it would actually use if it was given max_h as available height.
      // If we just used its max_h, that could increase the whole layout size
      // even if the element wouldn't actually use it.
      int height = GetWantedHeight(gravity, ps, ps.max_h);
      calculate_ps->max_h = std::max(calculate_ps->max_h, height);

      calculate_ps->size_dependency |= ps.size_dependency;
    }
  }

  if (calculate_ps) {
    // We just wanted to calculate preferred size, so return without layouting.
    *calculate_ps = GetRotatedPreferredSize(*calculate_ps, m_axis);
    return;
  }

  EL_IF_DEBUG_SETTING(util::DebugInfo::Setting::kLayoutSizing,
                      last_layout_time = util::GetTimeMS());

  // Pre Layout step (calculate distribution position).
  int missing_space = std::max(total_preferred_w - layout_rect.w, 0);
  int extra_space = std::max(layout_rect.w - total_preferred_w, 0);

  int offset = layout_rect.x;
  if (extra_space &&
      LayoutDistributionPosition(m_packed.layout_mode_dist_pos) !=
          LayoutDistributionPosition::kLeftTop) {
    // To calculate the offset we need to predict the used space. We can do that
    // by checking the distribution mode and total_max_pref_diff_w. That's how
    // much the elements could possible expand in the layout below.

    int used_space = total_preferred_w;
    if (LayoutDistribution(m_packed.layout_mode_dist) !=
        LayoutDistribution::kPreferred) {
      used_space += std::min(extra_space, total_max_pref_diff_w);
    }
    if (LayoutDistributionPosition(m_packed.layout_mode_dist_pos) ==
        LayoutDistributionPosition::kCenter) {
      offset += (layout_rect.w - used_space) / 2;
    } else {  // LayoutDistributionPosition::kRightBottom
      offset += layout_rect.w - used_space;
    }
  }

  // Layout
  int used_space = 0;
  for (Element* child = GetFirstInLayoutOrder(); child;
       child = GetNextInLayoutOrder(child)) {
    if (child->visibility() == Visibility::kGone) {
      continue;
    }

    const int ending_space = GetTrailingSpace(child, spacing);
    const PreferredSize ps =
        GetRotatedPreferredSize(child->GetPreferredSize(inner_sc), m_axis);
    const Gravity gravity = GetRotatedGravity(child->gravity(), m_axis);

    // Calculate width. May shrink if space is missing, or grow if we have extra
    // space.
    int width = ps.pref_w;
    if (missing_space && total_min_pref_diff_w) {
      int diff_w = ps.pref_w - ps.min_w;
      float factor = (float)diff_w / (float)total_min_pref_diff_w;
      int removed = (int)(missing_space * factor);
      removed = std::min(removed, diff_w);
      width -= removed;

      total_min_pref_diff_w -= diff_w;
      missing_space -= removed;
    } else if (extra_space && total_max_pref_diff_w &&
               QualifyForExpansion(gravity)) {
      int capped_max_w = std::min(layout_rect.w, ps.max_w);
      int diff_w = capped_max_w - ps.pref_w;
      float factor = (float)diff_w / (float)total_max_pref_diff_w;
      int added = (int)(extra_space * factor);
      added = std::min(added, diff_w);
      width += added;

      total_max_pref_diff_w -= capped_max_w - ps.pref_w;
      extra_space -= added;
    }

    // Calculate height.
    int available_height = layout_rect.h;
    int height = GetWantedHeight(gravity, ps, available_height);

    // Calculate position.
    int pos = layout_rect.y;
    switch (LayoutPosition(m_packed.layout_mode_pos)) {
      case LayoutPosition::kCenter:
        pos += (available_height - height) / 2;
        break;
      case LayoutPosition::kRightBottom:
        pos += available_height - height;
        break;
      case LayoutPosition::kGravity:
        if (any(gravity & Gravity::kTop) && any(gravity & Gravity::kBottom)) {
          pos += (available_height - height) / 2;
        } else if (any(gravity & Gravity::kBottom)) {
          pos += available_height - height;
        }
        break;
      default:  // LayoutPosition::kLeftTop
        break;
    };

    // Done! Set rect and increase used space.
    Rect rect(used_space + offset, pos, width, height);
    used_space += width + ending_space;

    child->set_rect(GetRotatedRect(rect, m_axis));
  }
  // Update overflow and overflow scroll.
  m_overflow = std::max(0, used_space - layout_rect.w);
  set_overflow_scroll(m_overflow_scroll);
}

PreferredSize LayoutBox::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  // Do a layout pass (without layouting) to check childrens preferences.
  PreferredSize ps;
  ValidateLayout(constraints, &ps);
  return ps;
}

bool LayoutBox::OnEvent(const Event& ev) {
  if (ev.type == EventType::kWheel && ev.modifierkeys == ModifierKeys::kNone) {
    int old_scroll = overflow_scroll();
    set_overflow_scroll(m_overflow_scroll +
                        ev.delta_y * util::GetPixelsPerLine());
    if (m_overflow_scroll != old_scroll) {
      return true;
    }
  }
  return Element::OnEvent(ev);
}

void LayoutBox::OnPaintChildren(const PaintProps& paint_props) {
  Rect padding_rect = this->padding_rect();
  if (padding_rect.empty()) return;

  // If we overflow the layout, apply clipping when painting children.
  Rect old_clip_rect;
  if (m_overflow) {
    // We only want clipping in one axis (the overflowing one) so we
    // don't damage any expanded skins on the other axis. Add some fluff.
    Rect clip_rect = padding_rect;
    const int fluff = 100;

    if (m_axis == Axis::kX) {
      clip_rect =
          clip_rect.Expand(m_overflow_scroll == 0 ? fluff : 0, fluff,
                           m_overflow_scroll == m_overflow ? fluff : 0, fluff);
    } else {
      clip_rect =
          clip_rect.Expand(fluff, m_overflow_scroll == 0 ? fluff : 0, fluff,
                           m_overflow_scroll == m_overflow ? fluff : 0);
    }

    old_clip_rect = Renderer::get()->set_clip_rect(clip_rect, true);

    EL_IF_DEBUG_SETTING(
        util::DebugInfo::Setting::kLayoutClipping,
        Renderer::get()->DrawRect(clip_rect, Color(255, 0, 0, 200)));
  }

  // Paint children.
  Element::OnPaintChildren(paint_props);

  // Paint fadeout image over the overflowed edges
  // to the indicate to used that it's overflowed.
  if (m_overflow && m_packed.paint_overflow_fadeout) {
    TBID skin_x, skin_y;
    if (m_axis == Axis::kX) {
      skin_x = TBIDC("Layout.fadeout_x");
    } else {
      skin_y = TBIDC("Layout.fadeout_y");
    }

    Skin::DrawEdgeFadeout(padding_rect, skin_x, skin_y, m_overflow_scroll,
                          m_overflow_scroll, m_overflow - m_overflow_scroll,
                          m_overflow - m_overflow_scroll);
  }

  // Restore clipping
  if (m_overflow) {
    Renderer::get()->set_clip_rect(old_clip_rect, false);
  }
}

void LayoutBox::OnProcess() {
  SizeConstraints sc(rect().w, rect().h);
  ValidateLayout(sc);
}

void LayoutBox::OnResized(int old_w, int old_h) {
  InvalidateLayout(InvalidationMode::kTargetOnly);
  SizeConstraints sc(rect().w, rect().h);
  ValidateLayout(sc);
}

void LayoutBox::OnInflateChild(Element* child) {
  // Do nothing since we're going to layout the child soon.
}

void LayoutBox::GetChildTranslation(int& x, int& y) const {
  if (m_axis == Axis::kX) {
    x = -m_overflow_scroll;
    y = 0;
  } else {
    x = 0;
    y = -m_overflow_scroll;
  }
}

void LayoutBox::ScrollTo(int x, int y) {
  set_overflow_scroll(m_axis == Axis::kX ? x : y);
}

Element::ScrollInfo LayoutBox::scroll_info() {
  ScrollInfo info;
  if (m_axis == Axis::kX) {
    info.max_x = m_overflow;
    info.x = m_overflow_scroll;
  } else {
    info.max_y = m_overflow;
    info.y = m_overflow_scroll;
  }
  return info;
}

}  // namespace elements
}  // namespace el
