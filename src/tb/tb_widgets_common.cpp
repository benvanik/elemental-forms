/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_widgets_common.h"

#include <algorithm>
#include <cassert>

#include "tb_widgets.h"
#include "tb_widgets_listener.h"

#include "tb/parsing/element_inflater.h"
#include "tb/resources/font_face.h"
#include "tb/resources/font_manager.h"
#include "tb/util/math.h"
#include "tb/util/metrics.h"

namespace tb {

ElementString::ElementString() = default;

int ElementString::GetWidth(Element* element) {
  return element->GetFont()->GetStringWidth(m_text);
}

int ElementString::GetHeight(Element* element) {
  return element->GetFont()->height();
}

void ElementString::Paint(Element* element, const Rect& rect,
                          const Color& color) {
  auto font = element->GetFont();
  int string_w = GetWidth(element);

  int x = rect.x;
  if (m_text_align == TextAlign::kRight) {
    x += rect.w - string_w;
  } else if (m_text_align == TextAlign::kCenter) {
    x += std::max(0, (rect.w - string_w) / 2);
  }
  int y = rect.y + (rect.h - GetHeight(element)) / 2;

  if (string_w <= rect.w) {
    font->DrawString(x, y, color, m_text);
  } else {
    // There's not enough room for the entire string
    // so cut it off and end with ellipsis (...)

    // const char *end = "…"; // 2026 HORIZONTAL ELLIPSIS
    // Some fonts seem to render ellipsis a lot uglier than three dots.
    const char* end = "...";

    int endw = font->GetStringWidth(end);
    int startw = 0;
    int startlen = 0;
    while (m_text[startlen]) {
      int new_startw = font->GetStringWidth(m_text, startlen);
      if (new_startw + endw > rect.w) {
        break;
      }
      startw = new_startw;
      startlen++;
    }
    startlen = std::max(0, startlen - 1);
    font->DrawString(x, y, color, m_text, startlen);
    font->DrawString(x + startw, y, color, end);
  }
}

void Label::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Label, Value::Type::kString, ElementZ::kTop);
}

Label::Label() { SetSkinBg(TBIDC("Label"), InvokeInfo::kNoCallbacks); }

void Label::OnInflate(const parsing::InflateInfo& info) {
  if (const char* text_align =
          info.node->GetValueString("text-align", nullptr)) {
    SetTextAlign(from_string(text_align, GetTextAlign()));
  }
  Element::OnInflate(info);
}

void Label::SetText(const char* text) {
  if (m_text.m_text.compare(text) == 0) return;
  m_cached_text_width = kTextWidthCacheNeedsUpdate;
  Invalidate();
  InvalidateLayout(InvalidationMode::kRecursive);
  m_text.SetText(text);
}

void Label::SetSqueezable(bool squeezable) {
  if (squeezable == m_squeezable) return;
  m_squeezable = squeezable;
  Invalidate();
  InvalidateLayout(InvalidationMode::kRecursive);
}

PreferredSize Label::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  PreferredSize ps;
  if (m_cached_text_width == kTextWidthCacheNeedsUpdate) {
    m_cached_text_width = m_text.GetWidth(this);
  }
  ps.pref_w = m_cached_text_width;
  ps.pref_h = ps.min_h = m_text.GetHeight(this);
  // If gravity pull both up and down, use default max_h (grow as much as
  // possible).
  // Otherwise it makes sense to only accept one line height.
  if (!(any(GetGravity() & Gravity::kTop) &&
        any(GetGravity() & Gravity::kBottom))) {
    ps.max_h = ps.pref_h;
  }
  if (!m_squeezable) {
    ps.min_w = ps.pref_w;
  }
  return ps;
}

void Label::OnFontChanged() {
  m_cached_text_width = kTextWidthCacheNeedsUpdate;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Label::OnPaint(const PaintProps& paint_props) {
  m_text.Paint(this, GetPaddingRect(), paint_props.text_color);
}

void Button::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Button, Value::Type::kNull, ElementZ::kBottom);
}

Button::Button() {
  SetIsFocusable(true);
  SetClickByKey(true);
  SetSkinBg(TBIDC("Button"), InvokeInfo::kNoCallbacks);
  AddChild(&m_layout);
  // Set the textfield gravity to all, even though it would display the same
  // with default gravity.
  // This will make the buttons layout expand if there is space available,
  // without forcing the parent
  // layout to grow to make the space available.
  m_textfield.SetGravity(Gravity::kAll);
  m_layout.AddChild(&m_textfield);
  m_layout.set_rect(GetPaddingRect());
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetPaintOverflowFadeout(false);
}

Button::~Button() {
  m_layout.RemoveChild(&m_textfield);
  RemoveChild(&m_layout);
}

void Button::OnInflate(const parsing::InflateInfo& info) {
  SetToggleMode(info.node->GetValueInt("toggle-mode", GetToggleMode()) ? true
                                                                       : false);
  Element::OnInflate(info);
}

void Button::SetText(const char* text) {
  m_textfield.SetText(text);
  UpdateLabelVisibility();
}

void Button::SetValue(int value) {
  if (value == GetValue()) return;
  SetState(Element::State::kPressed, value ? true : false);

  if (CanToggle()) {
    // Invoke a changed event.
    ElementEvent ev(EventType::kChanged);
    InvokeEvent(ev);
  }

  if (value && GetGroupID()) {
    BaseRadioCheckBox::UpdateGroupElements(this);
  }
}

int Button::GetValue() { return GetState(Element::State::kPressed); }

void Button::OnCaptureChanged(bool captured) {
  if (captured && m_auto_repeat_click) {
    PostMessageDelayed(TBIDC("auto_click"), nullptr,
                       kAutoClickFirstDelayMillis);
  } else if (!captured) {
    if (Message* msg = GetMessageByID(TBIDC("auto_click"))) {
      DeleteMessage(msg);
    }
  }
}

void Button::OnSkinChanged() { m_layout.set_rect(GetPaddingRect()); }

bool Button::OnEvent(const ElementEvent& ev) {
  if (CanToggle() && ev.type == EventType::kClick && ev.target == this) {
    WeakElementPointer this_element(this);

    // Toggle the value, if it's not a grouped element with value on.
    if (!(GetGroupID() && GetValue())) {
      SetValue(!GetValue());
    }

    if (!this_element.Get()) {
      return true;  // We got removed so we actually handled this event.
    }

    // Intentionally don't return true for this event. We want it to continue
    // propagating.
  }
  return Element::OnEvent(ev);
}

void Button::OnMessageReceived(Message* msg) {
  if (msg->message == TBIDC("auto_click")) {
    assert(captured_element == this);
    if (!cancel_click &&
        GetHitStatus(pointer_move_element_x, pointer_move_element_y) !=
            HitStatus::kNoHit) {
      ElementEvent ev(EventType::kClick, pointer_move_element_x,
                      pointer_move_element_y, true);
      captured_element->InvokeEvent(ev);
    }
    if (kAutoClickRepeattDelayMillis) {
      PostMessageDelayed(TBIDC("auto_click"), nullptr,
                         kAutoClickRepeattDelayMillis);
    }
  }
}

HitStatus Button::GetHitStatus(int x, int y) {
  // Never hit any of the children to the button. We always want to the button
  // itself.
  return Element::GetHitStatus(x, y) != HitStatus::kNoHit
             ? HitStatus::kHitNoChildren
             : HitStatus::kNoHit;
}

void Button::UpdateLabelVisibility() {
  // Auto-collapse the textfield if the text is empty and there are other
  // elements added apart from the textfield. This removes the extra spacing
  // added between the textfield and the other element.
  bool collapse_textfield = m_textfield.empty() &&
                            m_layout.GetFirstChild() != m_layout.GetLastChild();
  m_textfield.SetVisibilility(collapse_textfield ? Visibility::kGone
                                                 : Visibility::kVisible);
}

void Button::ButtonLayout::OnChildAdded(Element* child) {
  static_cast<Button*>(GetParent())->UpdateLabelVisibility();
}

void Button::ButtonLayout::OnChildRemove(Element* child) {
  static_cast<Button*>(GetParent())->UpdateLabelVisibility();
}

void LabelContainer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(LabelContainer, Value::Type::kString,
                               ElementZ::kBottom);
}

LabelContainer::LabelContainer() {
  AddChild(&m_layout);
  m_layout.AddChild(&m_textfield);
  m_layout.set_rect(GetPaddingRect());
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetLayoutDistributionPosition(LayoutDistributionPosition::kLeftTop);
}

LabelContainer::~LabelContainer() {
  m_layout.RemoveChild(&m_textfield);
  RemoveChild(&m_layout);
}

bool LabelContainer::OnEvent(const ElementEvent& ev) {
  // Get a element from the layout that isn't the textfield, or just bail out
  // if we only have the textfield.
  if (m_layout.GetFirstChild() == m_layout.GetLastChild()) {
    return false;
  }
  Element* click_target = m_layout.GetFirstChild() == &m_textfield
                              ? m_layout.GetLastChild()
                              : m_layout.GetFirstChild();
  // Invoke the event on it, as if it was invoked on the target itself.
  if (click_target && ev.target != click_target) {
    // Focus the target if we clicked the label.
    if (ev.type == EventType::kClick) {
      click_target->SetFocus(FocusReason::kPointer);
    }

    // Sync our pressed state with the click target. Special case for when we're
    // just about to lose it ourself (pointer is being released).
    bool pressed_state =
        any(ev.target->GetAutoState() & Element::State::kPressed);
    if (ev.type == EventType::kPointerUp || ev.type == EventType::kClick) {
      pressed_state = false;
    }

    click_target->SetState(Element::State::kPressed, pressed_state);

    ElementEvent target_ev(ev.type, ev.target_x - click_target->rect().x,
                           ev.target_y - click_target->rect().y, ev.touch,
                           ev.modifierkeys);
    return click_target->InvokeEvent(target_ev);
  }
  return false;
}

void SkinImage::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(SkinImage, Value::Type::kNull, ElementZ::kTop);
}

PreferredSize SkinImage::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredSize(constraints);
  // FIX: Make it stretched proportionally if shrunk.
  ps.max_w = ps.pref_w;
  ps.max_h = ps.pref_h;
  return ps;
}

void Separator::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Separator, Value::Type::kNull, ElementZ::kTop);
}

Separator::Separator() {
  SetSkinBg(TBIDC("Separator"), InvokeInfo::kNoCallbacks);
  SetState(Element::State::kDisabled, true);
}

void ProgressSpinner::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(ProgressSpinner, Value::Type::kInt,
                               ElementZ::kTop);
}

ProgressSpinner::ProgressSpinner() {
  SetSkinBg(TBIDC("ProgressSpinner"), InvokeInfo::kNoCallbacks);
  m_skin_fg.reset(TBIDC("ProgressSpinner.fg"));
}

void ProgressSpinner::SetValue(int value) {
  if (value == m_value) return;
  InvalidateSkinStates();
  assert(value >=
         0);  // If this happens, you probably have unballanced Begin/End calls.
  m_value = value;
  if (value > 0) {
    // Start animation.
    if (!GetMessageByID(TBID(1))) {
      m_frame = 0;
      PostMessageDelayed(TBID(1), nullptr, kSpinSpeed);
    }
  } else {
    // Stop animation.
    if (Message* msg = GetMessageByID(TBID(1))) {
      DeleteMessage(msg);
    }
  }
}

void ProgressSpinner::OnPaint(const PaintProps& paint_props) {
  if (IsRunning()) {
    auto e = resources::Skin::get()->GetSkinElement(m_skin_fg);
    if (e && e->bitmap) {
      int size = e->bitmap->Height();
      int num_frames = e->bitmap->Width() / e->bitmap->Height();
      int current_frame = m_frame % num_frames;
      graphics::Renderer::get()->DrawBitmap(
          GetPaddingRect(), Rect(current_frame * size, 0, size, size),
          e->bitmap);
    }
  }
}

void ProgressSpinner::OnMessageReceived(Message* msg) {
  m_frame++;
  Invalidate();
  // Keep animation running.
  PostMessageDelayed(TBID(1), nullptr, kSpinSpeed);
}

BaseRadioCheckBox::BaseRadioCheckBox() {
  SetIsFocusable(true);
  SetClickByKey(true);
}

// static
void BaseRadioCheckBox::UpdateGroupElements(Element* new_leader) {
  assert(new_leader->GetValue() && new_leader->GetGroupID());

  // Find the group root element.
  Element* group = new_leader;
  while (group && !group->GetIsGroupRoot() && group->GetParent()) {
    group = group->GetParent();
  }

  for (Element* child = group; child; child = child->GetNextDeep(group)) {
    if (child != new_leader &&
        child->GetGroupID() == new_leader->GetGroupID()) {
      child->SetValue(0);
    }
  }
}

void BaseRadioCheckBox::SetValue(int value) {
  if (m_value == value) return;
  m_value = value;

  SetState(Element::State::kSelected, value ? true : false);

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);

  if (value && GetGroupID()) UpdateGroupElements(this);
}

void CheckBox::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(CheckBox, Value::Type::kInt, ElementZ::kTop);
}

void RadioButton::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(RadioButton, Value::Type::kInt, ElementZ::kTop);
}

PreferredSize BaseRadioCheckBox::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = Element::OnCalculatePreferredSize(constraints);
  ps.min_w = ps.max_w = ps.pref_w;
  ps.min_h = ps.max_h = ps.pref_h;
  return ps;
}

bool BaseRadioCheckBox::OnEvent(const ElementEvent& ev) {
  if (ev.target == this && ev.type == EventType::kClick) {
    // Toggle the value, if it's not a grouped element with value on.
    if (!(GetGroupID() && GetValue())) {
      SetValue(!GetValue());
    }
  }
  return false;
}

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

Container::Container() {
  SetSkinBg(TBIDC("Container"), InvokeInfo::kNoCallbacks);
}

void Container::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Container, Value::Type::kNull, ElementZ::kTop);
}

void Mover::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Mover, Value::Type::kNull, ElementZ::kTop);
}

Mover::Mover() { SetSkinBg(TBIDC("Mover"), InvokeInfo::kNoCallbacks); }

bool Mover::OnEvent(const ElementEvent& ev) {
  Element* target = GetParent();
  if (!target) return false;
  if (ev.type == EventType::kPointerMove && captured_element == this) {
    int dx = ev.target_x - pointer_down_element_x;
    int dy = ev.target_y - pointer_down_element_y;
    Rect rect = target->rect().Offset(dx, dy);
    if (target->GetParent()) {
      // Apply limit.
      rect.x =
          util::Clamp(rect.x, -pointer_down_element_x,
                      target->GetParent()->rect().w - pointer_down_element_x);
      rect.y =
          util::Clamp(rect.y, -pointer_down_element_y,
                      target->GetParent()->rect().h - pointer_down_element_y);
    }
    target->set_rect(rect);
    return true;
  }
  return false;
}

void Resizer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Resizer, Value::Type::kNull, ElementZ::kTop);
}

Resizer::Resizer() { SetSkinBg(TBIDC("Resizer"), InvokeInfo::kNoCallbacks); }

HitStatus Resizer::GetHitStatus(int x, int y) {
  // Shave off some of the upper left diagonal half from the hit area.
  const int extra_hit_area = 3;
  if (x < rect().w - y - extra_hit_area) {
    return HitStatus::kNoHit;
  }
  return Element::GetHitStatus(x, y);
}

bool Resizer::OnEvent(const ElementEvent& ev) {
  Element* target = GetParent();
  if (!target) return false;
  if (ev.type == EventType::kPointerMove && captured_element == this) {
    int dx = ev.target_x - pointer_down_element_x;
    int dy = ev.target_y - pointer_down_element_y;
    Rect rect = target->rect();
    rect.w += dx;
    rect.h += dy;
    // Apply limit. We should not use minimum size since we can squeeze
    // the layout much more, and provide scroll/pan when smaller.
    rect.w = std::max(rect.w, 50);
    rect.h = std::max(rect.h, 50);
    target->set_rect(rect);
  } else {
    return false;
  }
  return true;
}

void Dimmer::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Dimmer, Value::Type::kNull, ElementZ::kTop);
}

Dimmer::Dimmer() {
  SetSkinBg(TBIDC("Dimmer"), InvokeInfo::kNoCallbacks);
  SetGravity(Gravity::kAll);
}

void Dimmer::OnAdded() {
  set_rect({0, 0, GetParent()->rect().w, GetParent()->rect().h});
}

}  // namespace tb
