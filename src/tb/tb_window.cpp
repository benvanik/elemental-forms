/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_window.h"

#include <algorithm>
#include <cassert>

namespace tb {

Window::Window() {
  SetSkinBg(TBIDC("Window"), InvokeInfo::kNoCallbacks);
  AddChild(&m_mover);
  AddChild(&m_resizer);
  m_mover.SetSkinBg(TBIDC("Window.mover"));
  m_mover.AddChild(&m_textfield);
  m_textfield.SetIgnoreInput(true);
  m_mover.AddChild(&m_close_button);
  m_close_button.SetSkinBg(TBIDC("Window.close"));
  m_close_button.SetIsFocusable(false);
  m_close_button.SetID(TBIDC("Window.close"));
  SetIsGroupRoot(true);
}

Window::~Window() {
  if (m_resizer.GetParent()) {
    RemoveChild(&m_resizer);
  }
  if (m_mover.GetParent()) {
    RemoveChild(&m_mover);
  }
  if (m_close_button.GetParent()) {
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
    new_w = Clamp(GetRect().w, ps.min_w, ps.max_w);
    new_h = Clamp(GetRect().h, ps.min_h, ps.max_h);
  }
  if (GetParent()) {
    new_w = std::min(new_w, GetParent()->GetRect().w);
    new_h = std::min(new_h, GetParent()->GetRect().h);
  }
  return Rect(GetRect().x, GetRect().y, new_w, new_h);
}

void Window::ResizeToFitContent(ResizeFit fit) {
  SetRect(GetResizeToFitContentRect(fit));
}

void Window::Close() { Die(); }

bool Window::IsActive() const { return GetState(SkinState::kSelected); }

Window* Window::GetTopMostOtherWindow(bool only_activable_windows) {
  Window* other_window = nullptr;
  Widget* sibling = GetParent()->GetLastChild();
  while (sibling && !other_window) {
    if (sibling != this) {
      other_window = TBSafeCast<Window>(sibling);
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
  if (!GetParent() || !any(m_settings & WindowSettings::kCanActivate)) {
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
  SetZ(WidgetZ::kTop);
  SetWindowActiveState(true);
  EnsureFocus();
}

bool Window::EnsureFocus() {
  // If we already have focus, we're done.
  if (focused_widget && IsAncestorOf(focused_widget)) {
    return true;
  }

  // Focus last focused widget (if we have one)
  bool success = false;
  if (m_last_focus.Get()) {
    success = m_last_focus.Get()->SetFocus(FocusReason::kUnknown);
  }
  // We didn't have one or failed, so try focus any child.
  if (!success) {
    success = SetFocusRecursive(FocusReason::kUnknown);
  }
  return success;
}

void Window::Deactivate() {
  if (!IsActive()) return;
  SetWindowActiveState(false);
}

void Window::SetWindowActiveState(bool active) {
  SetState(SkinState::kSelected, active);
  m_mover.SetState(SkinState::kSelected, active);
}

void Window::SetSettings(WindowSettings settings) {
  if (settings == m_settings) return;
  m_settings = settings;

  if (any(settings & WindowSettings::kTitleBar)) {
    if (!m_mover.GetParent()) {
      AddChild(&m_mover);
    }
  } else {
    if (m_mover.GetParent()) {
      RemoveChild(&m_mover);
    }
  }
  if (any(settings & WindowSettings::kResizable)) {
    if (!m_resizer.GetParent()) {
      AddChild(&m_resizer);
    }
  } else {
    if (m_resizer.GetParent()) {
      RemoveChild(&m_resizer);
    }
  }
  if (any(settings & WindowSettings::kCloseButton)) {
    if (!m_close_button.GetParent()) {
      m_mover.AddChild(&m_close_button);
    }
  } else {
    if (m_close_button.GetParent()) {
      m_mover.RemoveChild(&m_close_button);
    }
  }

  // FIX: invalidate layout / resize window!
  Invalidate();
}

int Window::GetTitleHeight() {
  if (any(m_settings & WindowSettings::kTitleBar)) {
    return m_mover.GetPreferredSize().pref_h;
  }
  return 0;
}

Rect Window::GetPaddingRect() {
  Rect padding_rect = Widget::GetPaddingRect();
  int title_height = GetTitleHeight();
  padding_rect.y += title_height;
  padding_rect.h -= title_height;
  return padding_rect;
}

PreferredSize Window::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = OnCalculatePreferredContentSize(constraints);

  // Add window skin padding
  if (SkinElement* e = GetSkinBgElement()) {
    ps.min_w += e->padding_left + e->padding_right;
    ps.pref_w += e->padding_left + e->padding_right;
    ps.min_h += e->padding_top + e->padding_bottom;
    ps.pref_h += e->padding_top + e->padding_bottom;
  }
  // Add window title bar height
  int title_height = GetTitleHeight();
  ps.min_h += title_height;
  ps.pref_h += title_height;
  return ps;
}

bool Window::OnEvent(const WidgetEvent& ev) {
  if (ev.target == &m_close_button) {
    if (ev.type == EventType::kClick) {
      Close();
    }
    return true;
  }
  return false;
}

void Window::OnAdded() {
  // If we was added last, call Activate to update status etc.
  if (GetParent()->GetLastChild() == this) {
    Activate();
  }
}

void Window::OnRemove() {
  Deactivate();

  // Active the top most other window
  if (Window* active_window = GetTopMostOtherWindow(true))
    active_window->Activate();
}

void Window::OnChildAdded(Widget* child) { m_resizer.SetZ(WidgetZ::kTop); }

void Window::OnResized(int old_w, int old_h) {
  // Apply gravity on children.
  Widget::OnResized(old_w, old_h);
  // Manually move our own decoration children.
  // FIX: Put a layout in the Mover so we can add things there nicely.
  int title_height = GetTitleHeight();
  m_mover.SetRect(Rect(0, 0, GetRect().w, title_height));
  PreferredSize ps = m_resizer.GetPreferredSize();
  m_resizer.SetRect(Rect(GetRect().w - ps.pref_w, GetRect().h - ps.pref_h,
                         ps.pref_w, ps.pref_h));
  Rect mover_rect = m_mover.GetPaddingRect();
  int button_size = mover_rect.h;
  m_close_button.SetRect(Rect(mover_rect.x + mover_rect.w - button_size,
                              mover_rect.y, button_size, button_size));
  if (any(m_settings & WindowSettings::kCloseButton)) {
    mover_rect.w -= button_size;
  }
  m_textfield.SetRect(mover_rect);
}

}  // namespace tb
