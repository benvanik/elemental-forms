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

#include "tb/util/math.h"
#include "tb/window.h"

namespace tb {

Window::Window() {
  set_background_skin(TBIDC("Window"), InvokeInfo::kNoCallbacks);
  AddChild(&m_mover);
  AddChild(&m_resizer);
  m_mover.set_background_skin(TBIDC("Window.mover"));
  m_mover.AddChild(&m_textfield);
  m_textfield.set_ignoring_input(true);
  m_mover.AddChild(&m_close_button);
  m_close_button.set_background_skin(TBIDC("Window.close"));
  m_close_button.set_focusable(false);
  m_close_button.set_id(TBIDC("Window.close"));
  set_group_root(true);
}

Window::~Window() {
  if (m_resizer.parent()) {
    RemoveChild(&m_resizer);
  }
  if (m_mover.parent()) {
    RemoveChild(&m_mover);
  }
  if (m_close_button.parent()) {
    m_mover.RemoveChild(&m_close_button);
  }
  m_mover.RemoveChild(&m_textfield);
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

bool Window::IsActive() const { return has_state(Element::State::kSelected); }

Window* Window::GetTopMostOtherWindow(bool only_activable_windows) {
  Window* other_window = nullptr;
  Element* sibling = parent()->last_child();
  while (sibling && !other_window) {
    if (sibling != this) {
      other_window = util::SafeCast<Window>(sibling);
    }
    if (only_activable_windows && other_window &&
        !any(other_window->m_settings & WindowSettings::kCanActivate)) {
      other_window = nullptr;
    }
    sibling = sibling->GetPrev();
  }
  return other_window;
}

void Window::Activate() {
  if (!parent() || !any(m_settings & WindowSettings::kCanActivate)) {
    return;
  }
  if (IsActive()) {
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
  if (m_last_focus.get()) {
    success = m_last_focus.get()->set_focus(FocusReason::kUnknown);
  }
  // We didn't have one or failed, so try focus any child.
  if (!success) {
    success = set_focus_recursive(FocusReason::kUnknown);
  }
  return success;
}

void Window::Deactivate() {
  if (!IsActive()) return;
  SetWindowActiveState(false);
}

void Window::SetWindowActiveState(bool active) {
  set_state(Element::State::kSelected, active);
  m_mover.set_state(Element::State::kSelected, active);
}

void Window::set_settings(WindowSettings settings) {
  if (settings == m_settings) return;
  m_settings = settings;

  if (any(settings & WindowSettings::kTitleBar)) {
    if (!m_mover.parent()) {
      AddChild(&m_mover);
    }
  } else {
    if (m_mover.parent()) {
      RemoveChild(&m_mover);
    }
  }
  if (any(settings & WindowSettings::kResizable)) {
    if (!m_resizer.parent()) {
      AddChild(&m_resizer);
    }
  } else {
    if (m_resizer.parent()) {
      RemoveChild(&m_resizer);
    }
  }
  if (any(settings & WindowSettings::kCloseButton)) {
    if (!m_close_button.parent()) {
      m_mover.AddChild(&m_close_button);
    }
  } else {
    if (m_close_button.parent()) {
      m_mover.RemoveChild(&m_close_button);
    }
  }

  // FIX: invalidate layout / resize window!
  Invalidate();
}

int Window::title_bar_height() {
  if (any(m_settings & WindowSettings::kTitleBar)) {
    return m_mover.GetPreferredSize().pref_h;
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
  if (ev.target == &m_close_button) {
    if (ev.type == EventType::kClick) {
      Close();
    }
    return true;
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

void Window::OnChildAdded(Element* child) { m_resizer.set_z(ElementZ::kTop); }

void Window::OnResized(int old_w, int old_h) {
  // Apply gravity on children.
  Element::OnResized(old_w, old_h);
  // Manually move our own decoration children.
  // FIX: Put a layout in the Mover so we can add things there nicely.
  int title_height = title_bar_height();
  m_mover.set_rect({0, 0, rect().w, title_height});
  PreferredSize ps = m_resizer.GetPreferredSize();
  m_resizer.set_rect(
      {rect().w - ps.pref_w, rect().h - ps.pref_h, ps.pref_w, ps.pref_h});
  Rect mover_rect = m_mover.padding_rect();
  int button_size = mover_rect.h;
  m_close_button.set_rect({mover_rect.x + mover_rect.w - button_size,
                           mover_rect.y, button_size, button_size});
  if (any(m_settings & WindowSettings::kCloseButton)) {
    mover_rect.w -= button_size;
  }
  m_textfield.set_rect(mover_rect);
}

}  // namespace tb
