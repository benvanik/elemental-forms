/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "tb/design/designer_window.h"
#include "tb/util/math.h"
#include "tb/window.h"

namespace tb {

Window::Window() {
  set_background_skin(TBIDC("Window"), InvokeInfo::kNoCallbacks);
  AddChild(&title_mover_);
  AddChild(&title_resizer_);
  title_mover_.set_background_skin(TBIDC("Window.mover"));
  title_mover_.AddChild(&title_label_);
  title_label_.set_ignoring_input(true);
  title_mover_.AddChild(&title_design_button_);
  title_design_button_.set_background_skin(TBIDC("Window.close"));
  title_design_button_.set_focusable(false);
  title_design_button_.set_id(TBIDC("Window.design"));
  title_mover_.AddChild(&title_close_button_);
  title_close_button_.set_background_skin(TBIDC("Window.close"));
  title_close_button_.set_focusable(false);
  title_close_button_.set_id(TBIDC("Window.close"));
  set_group_root(true);
}

Window::~Window() {
  title_resizer_.RemoveFromParent();
  title_mover_.RemoveFromParent();
  title_design_button_.RemoveFromParent();
  title_close_button_.RemoveFromParent();
  title_mover_.RemoveChild(&title_label_);
}

Rect Window::GetResizeToFitContentRect(ResizeFit fit) {
  PreferredSize ps = GetPreferredSize();
  int new_w = ps.pref_w;
  int new_h = ps.pref_h;
  if (fit == ResizeFit::kMinimal) {
    new_w = ps.min_w;
    new_h = ps.min_h;
  } else if (fit == ResizeFit::kCurrentOrNeeded) {
    new_w = util::Clamp(rect().w, ps.min_w, ps.max_w);
    new_h = util::Clamp(rect().h, ps.min_h, ps.max_h);
  }
  if (parent()) {
    new_w = std::min(new_w, parent()->rect().w);
    new_h = std::min(new_h, parent()->rect().h);
  }
  return Rect(rect().x, rect().y, new_w, new_h);
}

void Window::ResizeToFitContent(ResizeFit fit) {
  set_rect(GetResizeToFitContentRect(fit));
}

void Window::Close() { Die(); }

bool Window::is_active() const { return has_state(Element::State::kSelected); }

Window* Window::GetTopMostOtherWindow(bool only_activable_windows) {
  Window* other_window = nullptr;
  Element* sibling = parent()->last_child();
  while (sibling && !other_window) {
    if (sibling != this) {
      other_window = util::SafeCast<Window>(sibling);
    }
    if (only_activable_windows && other_window &&
        !any(other_window->window_settings_ & WindowSettings::kCanActivate)) {
      other_window = nullptr;
    }
    sibling = sibling->GetPrev();
  }
  return other_window;
}

void Window::Activate() {
  if (!parent() || !any(window_settings_ & WindowSettings::kCanActivate)) {
    return;
  }
  if (is_active()) {
    // Already active, but we may still have lost focus, so ensure it comes back
    // to us.
    EnsureFocus();
    return;
  }

  // Deactivate currently active window.
  Window* active_window = GetTopMostOtherWindow(true);
  if (active_window) {
    active_window->Deactivate();
  }

  // Activate this window.
  set_z(ElementZ::kTop);
  SetWindowActiveState(true);
  EnsureFocus();
}

bool Window::EnsureFocus() {
  // If we already have focus, we're done.
  if (focused_element && IsAncestorOf(focused_element)) {
    return true;
  }

  // Focus last focused element (if we have one)
  bool success = false;
  if (last_focus_element_.get()) {
    success = last_focus_element_.get()->set_focus(FocusReason::kUnknown);
  }
  // We didn't have one or failed, so try focus any child.
  if (!success) {
    success = set_focus_recursive(FocusReason::kUnknown);
  }
  return success;
}

void Window::Deactivate() {
  if (!is_active()) return;
  SetWindowActiveState(false);
}

void Window::SetWindowActiveState(bool active) {
  set_state(Element::State::kSelected, active);
  title_mover_.set_state(Element::State::kSelected, active);
}

void Window::set_settings(WindowSettings settings) {
  if (settings == window_settings_) return;
  window_settings_ = settings;

  if (any(settings & WindowSettings::kTitleBar)) {
    if (!title_mover_.parent()) {
      AddChild(&title_mover_);
    }
  } else {
    title_mover_.RemoveFromParent();
  }
  if (any(settings & WindowSettings::kResizable)) {
    if (!title_resizer_.parent()) {
      AddChild(&title_resizer_);
    }
  } else {
    title_resizer_.RemoveFromParent();
  }
  if (any(settings & WindowSettings::kDesignButton)) {
    if (!title_design_button_.parent()) {
      title_mover_.AddChild(&title_design_button_);
    }
  } else {
    title_design_button_.RemoveFromParent();
  }
  if (any(settings & WindowSettings::kCloseButton)) {
    if (!title_close_button_.parent()) {
      title_mover_.AddChild(&title_close_button_);
    }
  } else {
    title_close_button_.RemoveFromParent();
  }

  // FIX: invalidate layout / resize window!
  Invalidate();
}

int Window::title_bar_height() {
  if (any(window_settings_ & WindowSettings::kTitleBar)) {
    return title_mover_.GetPreferredSize().pref_h;
  }
  return 0;
}

Rect Window::padding_rect() {
  Rect padding_rect = Element::padding_rect();
  int title_height = title_bar_height();
  padding_rect.y += title_height;
  padding_rect.h -= title_height;
  return padding_rect;
}

PreferredSize Window::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = OnCalculatePreferredContentSize(constraints);

  // Add window skin padding
  if (auto e = background_skin_element()) {
    ps.min_w += e->padding_left + e->padding_right;
    ps.pref_w += e->padding_left + e->padding_right;
    ps.min_h += e->padding_top + e->padding_bottom;
    ps.pref_h += e->padding_top + e->padding_bottom;
  }
  // Add window title bar height
  int title_height = title_bar_height();
  ps.min_h += title_height;
  ps.pref_h += title_height;
  return ps;
}

bool Window::OnEvent(const Event& ev) {
  if (ev.target == &title_close_button_) {
    if (ev.type == EventType::kClick) {
      Close();
      return true;
    }
  } else if (ev.target == &title_design_button_) {
    if (ev.type == EventType::kClick) {
      OpenDesigner();
      return true;
    }
  }
  return Element::OnEvent(ev);
}

void Window::OnAdded() {
  // If we was added last, call Activate to update status etc.
  if (parent()->last_child() == this) {
    Activate();
  }
}

void Window::OnRemove() {
  Deactivate();

  // Active the top most other window
  if (Window* active_window = GetTopMostOtherWindow(true))
    active_window->Activate();
}

void Window::OnChildAdded(Element* child) {
  title_resizer_.set_z(ElementZ::kTop);
}

void Window::OnResized(int old_w, int old_h) {
  // Apply gravity on children.
  Element::OnResized(old_w, old_h);
  // Manually move our own decoration children.
  // FIX: Put a layout in the Mover so we can add things there nicely.
  int title_height = title_bar_height();
  title_mover_.set_rect({0, 0, rect().w, title_height});
  PreferredSize ps = title_resizer_.GetPreferredSize();
  title_resizer_.set_rect(
      {rect().w - ps.pref_w, rect().h - ps.pref_h, ps.pref_w, ps.pref_h});
  Rect title_mover_rect = title_mover_.padding_rect();
  int button_size = title_mover_rect.h;
  title_design_button_.set_rect(
      {title_mover_rect.x + title_mover_rect.w - button_size * 2,
       title_mover_rect.y, button_size, button_size});
  title_close_button_.set_rect(
      {title_mover_rect.x + title_mover_rect.w - button_size,
       title_mover_rect.y, button_size, button_size});
  if (any(window_settings_ & WindowSettings::kDesignButton)) {
    title_mover_rect.w -= button_size;
  }
  if (any(window_settings_ & WindowSettings::kCloseButton)) {
    title_mover_rect.w -= button_size;
  }
  title_label_.set_rect(title_mover_rect);
}

void Window::OpenDesigner() {
  auto designer_window = new design::DesignerWindow();
  designer_window->BindContent(this);
  designer_window->Show(this);
}

}  // namespace tb
