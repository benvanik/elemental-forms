/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/split_container.h"
#include "el/parsing/element_inflater.h"
#include "el/util/math.h"

namespace el {
namespace elements {

constexpr int kSplitterSize = 10;

void SplitContainer::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(SplitContainer, Value::Type::kInt,
                               ElementZ::kTop);
}

SplitContainer::SplitContainer() {
  set_gravity(Gravity::kAll);

  AddChild(&first_pane_);
  AddChild(&divider_);
  AddChild(&second_pane_);

  first_pane_.set_background_skin(TBIDC("SplitContainer.pane"));
  first_pane_.set_gravity(Gravity::kAll);
  second_pane_.set_background_skin(TBIDC("SplitContainer.pane"));
  second_pane_.set_gravity(Gravity::kAll);

  divider_.set_background_skin(TBIDC("SplitContainer.divider"));
  divider_.set_gravity(Gravity::kAll);
  divider_.set_focusable(true);
}

SplitContainer::~SplitContainer() {
  RemoveChild(&first_pane_);
  RemoveChild(&second_pane_);
  RemoveChild(&divider_);
}

void SplitContainer::OnInflate(const parsing::InflateInfo& info) {
  set_gravity(Gravity::kAll);
  if (auto axis_string = info.node->GetValueString("axis", nullptr)) {
    set_axis(from_string(axis_string, axis()));
  }
  if (auto fixed_string = info.node->GetValueString("fixed", nullptr)) {
    set_fixed_pane(from_string(fixed_string, fixed_pane()));
  }
  set_limits(info.node->GetValueInt("min", min_value()),
             info.node->GetValueInt("max", max_value()));
  initial_value_ = info.node->GetValueInt("value", util::kInvalidDimension);
  Element::OnInflate(info);
}

void SplitContainer::set_axis(Axis axis) {
  if (axis == axis_) return;
  axis_ = axis;
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void SplitContainer::set_fixed_pane(FixedPane fixed_pane) {
  if (fixed_pane == fixed_pane_) return;
  fixed_pane_ = fixed_pane;
  InvalidateLayout(InvalidationMode::kTargetOnly);
}

void SplitContainer::set_value(int value) {
  int clamped_value =
      util::Clamp(value, computed_min_value(), computed_max_value());
  if (clamped_value == value_) return;
  value_ = clamped_value;
  InvalidateLayout(InvalidationMode::kTargetOnly);
  Event ev(EventType::kChanged);
  InvokeEvent(ev);
}

int SplitContainer::computed_min_value() const {
  return min_value_ == util::kInvalidDimension ? 0 : min_value_;
}

int SplitContainer::computed_max_value() const {
  return axis_ == Axis::kX ? std::max(0, rect().h - kSplitterSize)
                           : std::max(0, rect().w - kSplitterSize);
}

void SplitContainer::set_limits(int min_value, int max_value) {
  if (min_value != util::kInvalidDimension &&
      max_value != util::kInvalidDimension) {
    min_value = std::min(min_value, max_value);
  }
  if (min_value == min_value_ && max_value == max_value_) {
    return;
  }
  min_value_ = min_value;
  max_value_ = max_value;
  set_value(value_);
}

void SplitContainer::OnProcess() {
  SizeConstraints sc(rect().w, rect().h);
  ValidateLayout(sc);
}

void SplitContainer::OnResized(int old_w, int old_h) {
  InvalidateLayout(InvalidationMode::kTargetOnly);
  SizeConstraints sc(rect().w, rect().h);
  ValidateLayout(sc);
}

void SplitContainer::OnChildAdded(Element* child) {
  if (child == &first_pane_ || child == &divider_ || child == &second_pane_) {
    return;
  }
  RemoveChild(child, InvokeInfo::kNoCallbacks);
  if (!first_pane_.first_child()) {
    first_pane_.AddChild(child);
  } else {
    second_pane_.AddChild(child);
  }
}

PreferredSize SplitContainer::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredContentSize(constraints);
  if (axis_ == Axis::kX) {
    ps.min_h += computed_min_value();
  } else {
    ps.min_w += computed_min_value();
  }
  ps.pref_w = std::max(ps.min_w, ps.pref_w);
  ps.pref_h = std::max(ps.min_h, ps.pref_h);
  return ps;
}

void SplitContainer::ValidateLayout(const SizeConstraints& constraints) {
  const Rect padding_rect = this->padding_rect();

  auto inner_sc = constraints.ConstrainByPadding(rect().w - padding_rect.w,
                                                 rect().h - padding_rect.h);

  // First layout since inflated?
  if (initial_value_ != util::kInvalidDimension) {
    set_value(initial_value_);
    initial_value_ = util::kInvalidDimension;
  }

  Rect first_rect;
  Rect divider_rect;
  Rect second_rect;
  if (axis_ == Axis::kX) {
    first_rect.x = divider_rect.x = second_rect.x = 0;
    first_rect.w = divider_rect.w = second_rect.w = inner_sc.available_w;
    if (fixed_pane_ == FixedPane::kFirst) {
      first_rect.y = 0;
      first_rect.h = value_;
      divider_rect.y = value_;
      divider_rect.h = kSplitterSize;
      second_rect.y = value_ + kSplitterSize;
      second_rect.h = inner_sc.available_h - value_ - kSplitterSize;
    } else {
      first_rect.y = 0;
      first_rect.h = inner_sc.available_h - value_ - kSplitterSize;
      divider_rect.y = inner_sc.available_h - value_ - kSplitterSize;
      divider_rect.h = kSplitterSize;
      second_rect.y = inner_sc.available_h - value_;
      second_rect.h = value_;
    }
  } else {
    first_rect.y = divider_rect.y = second_rect.y = 0;
    first_rect.h = divider_rect.h = second_rect.h = inner_sc.available_h;
    if (fixed_pane_ == FixedPane::kFirst) {
      first_rect.x = 0;
      first_rect.w = value_;
      divider_rect.x = value_;
      divider_rect.w = kSplitterSize;
      second_rect.x = value_ + kSplitterSize;
      second_rect.w = inner_sc.available_w - value_ - kSplitterSize;
    } else {
      first_rect.x = 0;
      first_rect.w = inner_sc.available_w - value_ - kSplitterSize;
      divider_rect.x = inner_sc.available_w - value_ - kSplitterSize;
      divider_rect.w = kSplitterSize;
      second_rect.x = inner_sc.available_w - value_;
      second_rect.w = value_;
    }
  }
  first_pane_.set_rect(first_rect);
  divider_.set_rect(divider_rect);
  second_pane_.set_rect(second_rect);
}

bool SplitContainer::Divider::OnEvent(const Event& ev) {
  auto parent = reinterpret_cast<SplitContainer*>(this->parent());
  if (!parent) {
    return false;
  }
  if (ev.type == EventType::kPointerMove && captured_element == this) {
    int dx = ev.target_x - pointer_down_element_x;
    int dy = ev.target_y - pointer_down_element_y;
    Rect r = rect().Offset(dx, dy);
    if (parent->axis() == Axis::kX) {
      parent->set_value(parent->fixed_pane() == FixedPane::kFirst
                            ? r.y
                            : -r.y + parent->rect().h);
    } else {
      parent->set_value(parent->fixed_pane() == FixedPane::kFirst
                            ? r.x
                            : -r.x + parent->rect().w);
    }
    return true;
  } else if (ev.type == EventType::kPointerDown && ev.count == 2) {
    if (parent->value() == parent->computed_min_value()) {
      parent->set_value(parent->computed_max_value());
    } else {
      parent->set_value(parent->computed_min_value());
    }
    return true;
  } else if (ev.type == EventType::kKeyDown) {
    int step = kSplitterSize;
    if ((ev.modifierkeys & ModifierKeys::kShift) == ModifierKeys::kShift) {
      step *= 4;
    }
    if (parent->fixed_pane() == FixedPane::kSecond) {
      step = -step;
    }
    if (ev.special_key == SpecialKey::kLeft ||
        ev.special_key == SpecialKey::kUp) {
      parent->set_value(parent->value() - step);
      return true;
    } else if (ev.special_key == SpecialKey::kRight ||
               ev.special_key == SpecialKey::kDown) {
      parent->set_value(parent->value() + step);
      return true;
    }
  } else if (ev.type == EventType::kKeyUp) {
    if (ev.special_key == SpecialKey::kLeft ||
        ev.special_key == SpecialKey::kUp ||
        ev.special_key == SpecialKey::kRight ||
        ev.special_key == SpecialKey::kDown) {
      return true;
    }
  }
  return Element::OnEvent(ev);
}

}  // namespace elements
}  // namespace el
