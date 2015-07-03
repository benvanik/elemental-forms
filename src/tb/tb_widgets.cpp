/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widgets.h"

#include <algorithm>
#include <cassert>

#include "tb_font_renderer.h"
#include "tb_renderer.h"
#include "tb_scroller.h"
#include "tb_system.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_window.h"

#ifdef TB_ALWAYS_SHOW_EDIT_FOCUS
#include "tb_text_box.h"
#endif  // TB_ALWAYS_SHOW_EDIT_FOCUS

namespace tb {

Widget* Widget::hovered_widget = nullptr;
Widget* Widget::captured_widget = nullptr;
Widget* Widget::focused_widget = nullptr;
int Widget::pointer_down_widget_x = 0;
int Widget::pointer_down_widget_y = 0;
int Widget::pointer_move_widget_x = 0;
int Widget::pointer_move_widget_y = 0;
bool Widget::cancel_click = false;
bool Widget::update_widget_states = true;
bool Widget::update_skin_states = true;
bool Widget::show_focus_state = false;

// One shot timer for long click event.
class LongClickTimer : private MessageHandler {
 public:
  LongClickTimer(Widget* widget, bool touch)
      : m_widget(widget), m_touch(touch) {
    PostMessageDelayed(TBIDC("LongClickTimer"), nullptr,
                       TBSystem::GetLongClickDelayMS());
  }
  void OnMessageReceived(Message* msg) override {
    assert(msg->message == TBIDC("LongClickTimer"));
    m_widget->MaybeInvokeLongClickOrContextMenu(m_touch);
  }

 private:
  Widget* m_widget;
  bool m_touch;
};

Widget::PaintProps::PaintProps() {
  // Set the default properties, used for the root widgets
  // calling InvokePaint. The base values for all inheritance.
  text_color = g_tb_skin->GetDefaultTextColor();
}

Widget::Widget() = default;

Widget::~Widget() {
  assert(!m_parent);  // A widget must be removed from parent before deleted.
  m_packed.is_dying = true;

  if (this == hovered_widget) {
    hovered_widget = nullptr;
  }
  if (this == captured_widget) {
    captured_widget = nullptr;
  }
  if (this == focused_widget) {
    focused_widget = nullptr;
  }

  WidgetListener::InvokeWidgetDelete(this);
  DeleteAllChildren();

  delete m_scroller;
  delete m_layout_params;

  StopLongClickTimer();

  assert(!m_listeners
              .HasLinks());  // There's still listeners added to this widget!
}

void Widget::SetRect(const Rect& rect) {
  if (m_rect.Equals(rect)) return;

  Rect old_rect = m_rect;
  m_rect = rect;

  if (old_rect.w != m_rect.w || old_rect.h != m_rect.h) {
    OnResized(old_rect.w, old_rect.h);
  }

  Invalidate();
}

void Widget::Invalidate() {
  if (!GetVisibilityCombined() && !m_rect.IsEmpty()) {
    return;
  }
  Widget* tmp = this;
  while (tmp) {
    tmp->OnInvalid();
    tmp = tmp->m_parent;
  }
}

void Widget::InvalidateStates() {
  update_widget_states = true;
  InvalidateSkinStates();
}

void Widget::InvalidateSkinStates() { update_skin_states = true; }

void Widget::Die() {
  if (m_packed.is_dying) {
    return;
  }
  m_packed.is_dying = true;
  OnDie();
  if (!WidgetListener::InvokeWidgetDying(this)) {
    // No one was interested, so die immediately.
    if (m_parent) {
      m_parent->RemoveChild(this);
    }
    delete this;
  }
}

Widget* Widget::GetWidgetByIDInternal(const TBID& id,
                                      const tb_type_id_t type_id) {
  if (m_id == id && (!type_id || IsOfTypeId(type_id))) {
    return this;
  }
  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (Widget* sub_child = child->GetWidgetByIDInternal(id, type_id)) {
      return sub_child;
    }
  }
  return nullptr;
}

std::string Widget::GetTextByID(const TBID& id) {
  if (Widget* widget = GetWidgetByID(id)) {
    return widget->GetText();
  }
  return "";
}

int Widget::GetValueByID(const TBID& id) {
  if (Widget* widget = GetWidgetByID(id)) {
    return widget->GetValue();
  }
  return 0;
}

void Widget::SetID(const TBID& id) {
  m_id = id;
  InvalidateSkinStates();
}

void Widget::SetStateRaw(SkinState state) {
  if (m_state == state) return;
  m_state = state;
  Invalidate();
  InvalidateSkinStates();
}

void Widget::SetState(SkinState state, bool on) {
  SetStateRaw(on ? m_state | state : m_state & ~state);
}

SkinState Widget::GetAutoState() const {
  SkinState state = m_state;
  bool add_pressed_state =
      !cancel_click && this == captured_widget && this == hovered_widget;
  if (add_pressed_state) {
    state |= SkinState::kPressed;
  }
  if (this == hovered_widget &&
      (!m_packed.no_automatic_hover_state || add_pressed_state)) {
    state |= SkinState::kHovered;
  }
  if (this == focused_widget && show_focus_state) {
    state |= SkinState::kFocused;
  }
#ifdef TB_ALWAYS_SHOW_EDIT_FOCUS
  else if (this == focused_widget && IsOfType<TextBox>()) {
    state |= SkinState::kFocused;
  }
#endif
  return state;
}

// static
void Widget::SetAutoFocusState(bool on) {
  if (show_focus_state == on) {
    return;
  }
  show_focus_state = on;
  if (focused_widget) {
    focused_widget->Invalidate();
  }
}

void Widget::SetOpacity(float opacity) {
  opacity = Clamp(opacity, 0.f, 1.f);
  if (m_opacity == opacity) {
    return;
  }
  if (opacity == 0) {
    // Invalidate after setting opacity 0 will do nothing.
    Invalidate();
  }
  m_opacity = opacity;
  Invalidate();
}

void Widget::SetVisibilility(Visibility vis) {
  if (m_packed.visibility == int(vis)) {
    return;
  }

  // Invalidate after making it invisible will do nothing.
  if (vis != Visibility::kVisible) {
    Invalidate();
  }
  if (vis == Visibility::kGone) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  Visibility old_vis = GetVisibility();
  m_packed.visibility = int(vis);

  Invalidate();
  if (old_vis == Visibility::kGone) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  OnVisibilityChanged();
}

Visibility Widget::GetVisibility() const {
  return static_cast<Visibility>(m_packed.visibility);
}

bool Widget::GetVisibilityCombined() const {
  const Widget* tmp = this;
  while (tmp) {
    if (tmp->GetOpacity() == 0 ||
        tmp->GetVisibility() != Visibility::kVisible) {
      return false;
    }
    tmp = tmp->m_parent;
  }
  return true;
}

bool Widget::GetDisabled() const {
  const Widget* tmp = this;
  while (tmp) {
    if (tmp->GetState(SkinState::kDisabled)) {
      return true;
    }
    tmp = tmp->m_parent;
  }
  return false;
}

void Widget::AddChild(Widget* child, WidgetZ z, InvokeInfo info) {
  AddChildRelative(
      child, z == WidgetZ::kTop ? WidgetZRel::kAfter : WidgetZRel::kBefore,
      nullptr, info);
}

void Widget::AddChildRelative(Widget* child, WidgetZRel z, Widget* reference,
                              InvokeInfo info) {
  assert(!child->m_parent);
  child->m_parent = this;

  if (reference) {
    if (z == WidgetZRel::kBefore) {
      m_children.AddBefore(child, reference);
    } else {
      m_children.AddAfter(child, reference);
    }
  } else {
    // If there is no reference widget, before means first and after means last.
    if (z == WidgetZRel::kBefore) {
      m_children.AddFirst(child);
    } else {
      m_children.AddLast(child);
    }
  }

  if (info == InvokeInfo::kNormal) {
    OnChildAdded(child);
    child->OnAdded();
    WidgetListener::InvokeWidgetAdded(this, child);
  }
  InvalidateLayout(InvalidationMode::kRecursive);
  Invalidate();
  InvalidateSkinStates();
}

void Widget::RemoveChild(Widget* child, InvokeInfo info) {
  assert(child->m_parent);

  if (info == InvokeInfo::kNormal) {
    // If we're not being deleted and delete the focused widget, try
    // to keep the focus in this widget by moving it to the next widget.
    if (!m_packed.is_dying && child == focused_widget) {
      m_parent->MoveFocus(true);
    }

    OnChildRemove(child);
    child->OnRemove();
    WidgetListener::InvokeWidgetRemove(this, child);
  }

  m_children.Remove(child);
  child->m_parent = nullptr;

  InvalidateLayout(InvalidationMode::kRecursive);
  Invalidate();
  InvalidateSkinStates();
}

void Widget::DeleteAllChildren() {
  while (Widget* child = GetFirstChild()) {
    RemoveChild(child);
    delete child;
  }
}

void Widget::SetZ(WidgetZ z) {
  if (!m_parent) return;
  if (z == WidgetZ::kTop && this == m_parent->m_children.GetLast()) {
    return;  // Already at the top
  }
  if (z == WidgetZ::kBottom && this == m_parent->m_children.GetFirst()) {
    return;  // Already at the top
  }
  Widget* parent = m_parent;
  parent->RemoveChild(this, InvokeInfo::kNoCallbacks);
  parent->AddChild(this, z, InvokeInfo::kNoCallbacks);
}

void Widget::SetGravity(Gravity g) {
  if (m_gravity == g) return;
  m_gravity = g;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Widget::SetSkinBg(const TBID& skin_bg, InvokeInfo info) {
  if (skin_bg == m_skin_bg) return;

  // Set the skin and m_skin_bg_expected. During InvokeProcess, we will detect
  // if any widget get a different element due to conditions and strong
  // override.
  // If that happens, OnSkinChanged will be called and m_skin_bg_expected
  // updated to match that override.
  m_skin_bg = skin_bg;
  m_skin_bg_expected = skin_bg;

  Invalidate();
  InvalidateSkinStates();
  InvalidateLayout(InvalidationMode::kRecursive);

  if (info == InvokeInfo::kNormal) {
    OnSkinChanged();
  }
}

SkinElement* Widget::GetSkinBgElement() {
  WidgetSkinConditionContext context(this);
  SkinState state = GetAutoState();
  return g_tb_skin->GetSkinElementStrongOverride(
      m_skin_bg, static_cast<SkinState>(state), context);
}

Widget* Widget::FindScrollableWidget(bool scroll_x, bool scroll_y) {
  Widget* candidate = this;
  while (candidate) {
    ScrollInfo scroll_info = candidate->GetScrollInfo();
    if ((scroll_x && scroll_info.CanScrollX()) ||
        (scroll_y && scroll_info.CanScrollY())) {
      return candidate;
    }
    candidate = candidate->GetParent();
  }
  return nullptr;
}

Scroller* Widget::FindStartedScroller() {
  Widget* candidate = this;
  while (candidate) {
    if (candidate->m_scroller && candidate->m_scroller->IsStarted()) {
      return candidate->m_scroller;
    }
    candidate = candidate->GetParent();
  }
  return nullptr;
}

Scroller* Widget::GetReadyScroller(bool scroll_x, bool scroll_y) {
  if (Scroller* scroller = FindStartedScroller()) return scroller;
  // We didn't have any active scroller, so create one for the nearest
  // scrollable parent.
  if (Widget* scrollable_widget = FindScrollableWidget(scroll_x, scroll_y)) {
    return scrollable_widget->GetScroller();
  }
  return nullptr;
}

Scroller* Widget::GetScroller() {
  if (!m_scroller) {
    m_scroller = new Scroller(this);
  }
  return m_scroller;
}

void Widget::ScrollToSmooth(int x, int y) {
  ScrollInfo info = GetScrollInfo();
  int dx = x - info.x;
  int dy = y - info.y;
  if (Scroller* scroller = GetReadyScroller(dx != 0, dy != 0)) {
    scroller->OnScrollBy(dx, dy, false);
  }
}

void Widget::ScrollBySmooth(int dx, int dy) {
  // Clip the values to the scroll limits, so we don't
  // scroll any parents.
  // int x = Clamp(info.x + dx, info.min_x, info.max_x);
  // int y = Clamp(info.y + dy, info.min_y, info.max_y);
  // dx = x - info.x;
  // dy = y - info.y;
  if (!dx && !dy) return;

  if (Scroller* scroller = GetReadyScroller(dx != 0, dy != 0)) {
    scroller->OnScrollBy(dx, dy, true);
  }
}

void Widget::ScrollBy(int dx, int dy) {
  ScrollInfo info = GetScrollInfo();
  ScrollTo(info.x + dx, info.y + dy);
}

void Widget::ScrollByRecursive(int& dx, int& dy) {
  Widget* tmp = this;
  while (tmp) {
    ScrollInfo old_info = tmp->GetScrollInfo();
    tmp->ScrollTo(old_info.x + dx, old_info.y + dy);
    ScrollInfo new_info = tmp->GetScrollInfo();
    dx -= new_info.x - old_info.x;
    dy -= new_info.y - old_info.y;
    if (!dx && !dy) {
      break;
    }
    tmp = tmp->m_parent;
  }
}

void Widget::ScrollIntoViewRecursive() {
  Rect scroll_to_rect = m_rect;
  Widget* tmp = this;
  while (tmp->m_parent) {
    tmp->m_parent->ScrollIntoView(scroll_to_rect);
    scroll_to_rect.x += tmp->m_parent->m_rect.x;
    scroll_to_rect.y += tmp->m_parent->m_rect.y;
    tmp = tmp->m_parent;
  }
}

void Widget::ScrollIntoView(const Rect& rect) {
  const ScrollInfo info = GetScrollInfo();
  int new_x = info.x;
  int new_y = info.y;

  const Rect visible_rect = GetPaddingRect().Offset(info.x, info.y);

  if (rect.y <= visible_rect.y) {
    new_y = rect.y;
  } else if (rect.y + rect.h > visible_rect.y + visible_rect.h) {
    new_y = rect.y + rect.h - visible_rect.h;
  }

  if (rect.x <= visible_rect.x) {
    new_x = rect.x;
  } else if (rect.x + rect.w > visible_rect.x + visible_rect.w) {
    new_x = rect.x + rect.w - visible_rect.w;
  }

  ScrollTo(new_x, new_y);
}

bool Widget::SetFocus(FocusReason reason, InvokeInfo info) {
  if (focused_widget == this) return true;
  if (GetDisabled() || !GetIsFocusable() || !GetVisibilityCombined() ||
      GetIsDying()) {
    return false;
  }

  // Update windows last focus
  Window* window = GetParentWindow();
  if (window) {
    window->SetLastFocus(this);
    // If not active, just return. We should get focus when the window is
    // activated.
    // Exception for windows that doesn't activate. They may contain focusable
    // widgets.
    if (!window->IsActive() &&
        any(window->GetSettings() & WindowSettings::kCanActivate)) {
      return true;
    }
  }

  if (focused_widget) {
    focused_widget->Invalidate();
    focused_widget->InvalidateSkinStates();
  }

  WeakWidgetPointer old_focus(focused_widget);
  focused_widget = this;

  Invalidate();
  InvalidateSkinStates();

  if (reason == FocusReason::kNavigation) {
    ScrollIntoViewRecursive();
  }

  if (info == InvokeInfo::kNormal) {
    // A lot of weird bugs could happen if people mess with focus from
    // OnFocusChanged.
    // Take some precaution and detect if it change again after
    // OnFocusChanged(false).
    if (Widget* old = old_focus.Get()) {
      // The currently focused widget still has the pressed state set by the
      // emulated click (by keyboard), so unset it before we unfocus it so it's
      // not stuck in pressed state.
      if (old->m_packed.has_key_pressed_state) {
        old->SetState(SkinState::kPressed, false);
        old->m_packed.has_key_pressed_state = false;
      }
      old->OnFocusChanged(false);
    }
    if (old_focus.Get()) {
      WidgetListener::InvokeWidgetFocusChanged(old_focus.Get(), false);
    }
    if (focused_widget && focused_widget == this) {
      focused_widget->OnFocusChanged(true);
    }
    if (focused_widget && focused_widget == this) {
      WidgetListener::InvokeWidgetFocusChanged(focused_widget, true);
    }
  }
  return true;
}

bool Widget::SetFocusRecursive(FocusReason reason) {
  // Search for a child widget that accepts focus.
  Widget* child = GetFirstChild();
  while (child) {
    if (child->SetFocus(FocusReason::kUnknown)) {
      return true;
    }
    child = child->GetNextDeep(this);
  }
  return false;
}

bool Widget::MoveFocus(bool forward) {
  Widget* origin = focused_widget;
  if (!origin) {
    origin = this;
  }

  Widget* root = origin->GetParentWindow();
  if (!root) {
    root = origin->GetParentRoot();
  }

  Widget* current = origin;
  while (current) {
    current = forward ? current->GetNextDeep() : current->GetPrevDeep();
    // Wrap around if we reach the end/beginning.
    if (!current || !root->IsAncestorOf(current)) {
      current = forward ? root->GetFirstChild() : root->GetLastLeaf();
    }
    // Break if we reached the origin again (we're not finding anything else).
    if (current == origin) {
      break;
    }
    // Try to focus what we found.
    if (current && current->SetFocus(FocusReason::kNavigation)) {
      return true;
    }
  }
  return false;
}

Widget* Widget::GetNextDeep(const Widget* bounding_ancestor) const {
  if (m_children.GetFirst()) {
    return GetFirstChild();
  }
  for (const Widget* widget = this; widget != bounding_ancestor;
       widget = widget->m_parent) {
    if (widget->next) {
      return widget->GetNext();
    }
  }
  return nullptr;
}

Widget* Widget::GetPrevDeep() const {
  if (!prev) return m_parent;
  Widget* widget = GetPrev();
  while (widget->m_children.GetLast()) {
    widget = widget->GetLastChild();
  }
  return widget;
}

Widget* Widget::GetLastLeaf() const {
  if (Widget* widget = GetLastChild()) {
    while (widget->GetLastChild()) {
      widget = widget->GetLastChild();
    }
    return widget;
  }
  return nullptr;
}

bool Widget::GetIsInteractable() const {
  return !(m_opacity == 0 || GetIgnoreInput() ||
           GetState(SkinState::kDisabled) || GetIsDying() ||
           GetVisibility() != Visibility::kVisible);
}

HitStatus Widget::GetHitStatus(int x, int y) {
  if (!GetIsInteractable()) return HitStatus::kNoHit;
  return x >= 0 && y >= 0 && x < m_rect.w && y < m_rect.h ? HitStatus::kHit
                                                          : HitStatus::kNoHit;
}

Widget* Widget::GetWidgetAt(int x, int y, bool include_children) const {
  int child_translation_x, child_translation_y;
  GetChildTranslation(child_translation_x, child_translation_y);
  x -= child_translation_x;
  y -= child_translation_y;

  Widget* tmp = GetFirstChild();
  Widget* last_match = nullptr;
  while (tmp) {
    HitStatus hit_status =
        tmp->GetHitStatus(x - tmp->m_rect.x, y - tmp->m_rect.y);
    if (hit_status != HitStatus::kNoHit) {
      if (include_children && hit_status != HitStatus::kHitNoChildren) {
        last_match = tmp->GetWidgetAt(x - tmp->m_rect.x, y - tmp->m_rect.y,
                                      include_children);
        if (!last_match) {
          last_match = tmp;
        }
      } else {
        last_match = tmp;
      }
    }
    tmp = tmp->GetNext();
  }
  return last_match;
}

Widget* Widget::GetChildFromIndex(int index) const {
  int i = 0;
  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (i++ == index) return child;
  }
  return nullptr;
}

int Widget::GetIndexFromChild(Widget* child) const {
  assert(child->GetParent() == this);
  int i = 0;
  for (Widget* tmp = GetFirstChild(); tmp; tmp = tmp->GetNext(), i++) {
    if (tmp == child) {
      return i;
    }
  }
  return -1;  // Should not happen!
}

bool Widget::IsAncestorOf(Widget* other_widget) const {
  while (other_widget) {
    if (other_widget == this) {
      return true;
    }
    other_widget = other_widget->m_parent;
  }
  return false;
}

bool Widget::IsEventDestinationFor(Widget* other_widget) const {
  while (other_widget) {
    if (other_widget == this) {
      return true;
    }
    other_widget = other_widget->GetEventDestination();
  }
  return false;
}

Widget* Widget::GetParentRoot() {
  Widget* tmp = this;
  while (tmp->m_parent) {
    tmp = tmp->m_parent;
  }
  return tmp;
}

Window* Widget::GetParentWindow() {
  Widget* tmp = this;
  while (tmp && !tmp->IsOfType<Window>()) {
    tmp = tmp->m_parent;
  }
  return static_cast<Window*>(tmp);
}

void Widget::AddListener(WidgetListener* listener) {
  m_listeners.AddLast(listener);
}

void Widget::RemoveListener(WidgetListener* listener) {
  m_listeners.Remove(listener);
}

bool Widget::HasListener(WidgetListener* listener) const {
  return m_listeners.ContainsLink(listener);
}

void Widget::OnPaintChildren(const PaintProps& paint_props) {
  if (!m_children.GetFirst()) return;

  // Translate renderer with child translation.
  int child_translation_x, child_translation_y;
  GetChildTranslation(child_translation_x, child_translation_y);
  g_renderer->Translate(child_translation_x, child_translation_y);

  Rect clip_rect = g_renderer->GetClipRect();

  // Invoke paint on all children that are in the current visible rect.
  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (clip_rect.Intersects(child->m_rect)) {
      child->InvokePaint(paint_props);
    }
  }

  // Invoke paint of overlay elements on all children that are in the current
  // visible rect.
  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (clip_rect.Intersects(child->m_rect) &&
        child->GetVisibility() == Visibility::kVisible) {
      SkinElement* skin_element = child->GetSkinBgElement();
      if (skin_element && skin_element->HasOverlayElements()) {
        // Update the renderer with the widgets opacity.
        SkinState state = child->GetAutoState();
        float old_opacity = g_renderer->GetOpacity();
        float opacity =
            old_opacity * child->CalculateOpacityInternal(state, skin_element);
        if (opacity > 0) {
          g_renderer->SetOpacity(opacity);

          WidgetSkinConditionContext context(child);
          g_tb_skin->PaintSkinOverlay(child->m_rect, skin_element,
                                      static_cast<SkinState>(state), context);

          g_renderer->SetOpacity(old_opacity);
        }
      }
    }
  }

  // Draw generic focus skin if the focused widget is one of the children, and
  // the skin doesn't have a skin state for focus which would already be
  // painted.
  if (focused_widget && focused_widget->m_parent == this) {
    WidgetSkinConditionContext context(focused_widget);
    SkinElement* skin_element = focused_widget->GetSkinBgElement();
    if (!skin_element ||
        !skin_element->HasState(SkinState::kFocused, context)) {
      SkinState state = focused_widget->GetAutoState();
      if (any(state & SkinState::kFocused)) {
        g_tb_skin->PaintSkin(focused_widget->m_rect, TBIDC("generic_focus"),
                             static_cast<SkinState>(state), context);
      }
    }
  }

  g_renderer->Translate(-child_translation_x, -child_translation_y);
}

void Widget::OnResized(int old_w, int old_h) {
  int dw = m_rect.w - old_w;
  int dh = m_rect.h - old_h;
  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (child->GetVisibility() == Visibility::kGone) continue;
    Rect rect = child->m_rect;
    if (any(child->m_gravity & Gravity::kLeft) &&
        any(child->m_gravity & Gravity::kRight)) {
      rect.w += dw;
    } else if (any(child->m_gravity & Gravity::kRight)) {
      rect.x += dw;
    }
    if (any(child->m_gravity & Gravity::kTop) &&
        any(child->m_gravity & Gravity::kBottom)) {
      rect.h += dh;
    } else if (any(child->m_gravity & Gravity::kBottom)) {
      rect.y += dh;
    }
    child->SetRect(rect);
  }
}

void Widget::OnInflateChild(Widget* child) {
  if (child->GetVisibility() == Visibility::kGone) return;

  // If the child pull towards only one edge (per axis), stick to that edge
  // and use the preferred size. Otherwise fill up all available space.
  Rect padding_rect = GetPaddingRect();
  Rect child_rect = padding_rect;
  Gravity gravity = child->GetGravity();
  bool fill_x = any(gravity & Gravity::kLeft) && any(gravity & Gravity::kRight);
  bool fill_y = any(gravity & Gravity::kTop) && any(gravity & Gravity::kBottom);
  if (!fill_x || !fill_y) {
    PreferredSize ps = child->GetPreferredSize();
    if (!fill_x) {
      child_rect.w = ps.pref_w;
      if (any(gravity & Gravity::kRight)) {
        child_rect.x = padding_rect.x + padding_rect.w - child_rect.w;
      }
    }
    if (!fill_y) {
      child_rect.h = ps.pref_h;
      if (any(gravity & Gravity::kBottom)) {
        child_rect.y = padding_rect.y + padding_rect.h - child_rect.h;
      }
    }
  }
  child->SetRect(child_rect);
}

Rect Widget::GetPaddingRect() {
  Rect padding_rect(0, 0, m_rect.w, m_rect.h);
  if (SkinElement* e = GetSkinBgElement()) {
    padding_rect.x += e->padding_left;
    padding_rect.y += e->padding_top;
    padding_rect.w -= e->padding_left + e->padding_right;
    padding_rect.h -= e->padding_top + e->padding_bottom;
  }
  return padding_rect;
}

PreferredSize Widget::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  // The default preferred size is calculated to satisfy the children
  // in the best way. Since this is the default, it's probably not a
  // layouting widget and children are resized purely by gravity.

  // Allow this widget a larger maximum if our gravity wants both ways,
  // otherwise don't grow more than the largest child.
  bool apply_max_w =
      !any(m_gravity & Gravity::kLeft) && any(m_gravity & Gravity::kRight);
  bool apply_max_h =
      !any(m_gravity & Gravity::kTop) && any(m_gravity & Gravity::kBottom);
  bool has_layouting_children = false;
  PreferredSize ps;

  SkinElement* bg_skin = GetSkinBgElement();
  int horizontal_padding =
      bg_skin ? bg_skin->padding_left + bg_skin->padding_right : 0;
  int vertical_padding =
      bg_skin ? bg_skin->padding_top + bg_skin->padding_bottom : 0;
  SizeConstraints inner_sc =
      constraints.ConstrainByPadding(horizontal_padding, vertical_padding);

  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (child->GetVisibility() == Visibility::kGone) {
      continue;
    }
    if (!has_layouting_children) {
      has_layouting_children = true;
      if (apply_max_w) ps.max_w = 0;
      if (apply_max_h) ps.max_h = 0;
    }
    PreferredSize child_ps = child->GetPreferredSize(inner_sc);
    ps.pref_w = std::max(ps.pref_w, child_ps.pref_w);
    ps.pref_h = std::max(ps.pref_h, child_ps.pref_h);
    ps.min_w = std::max(ps.min_w, child_ps.min_w);
    ps.min_h = std::max(ps.min_h, child_ps.min_h);
    if (apply_max_w) {
      ps.max_w = std::max(ps.max_w, child_ps.max_w);
    }
    if (apply_max_h) {
      ps.max_h = std::max(ps.max_h, child_ps.max_h);
    }
    ps.size_dependency |= child_ps.size_dependency;
  }

  return ps;
}

PreferredSize Widget::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = OnCalculatePreferredContentSize(constraints);
  assert(ps.pref_w >= ps.min_w);
  assert(ps.pref_h >= ps.min_h);

  if (SkinElement* e = GetSkinBgElement()) {
    // Override the widgets preferences with skin attributes that has been
    // specified.
    // If not set by the widget, calculate based on the intrinsic size of the
    // skin.

    const int skin_intrinsic_w = e->GetIntrinsicWidth();
    if (e->GetPrefWidth() != kSkinValueNotSpecified) {
      ps.pref_w = e->GetPrefWidth();
    } else if (ps.pref_w == 0 && skin_intrinsic_w != kSkinValueNotSpecified) {
      ps.pref_w = skin_intrinsic_w;
    } else {
      // Grow by padding to get the preferred size from preferred content size.
      ps.min_w += e->padding_left + e->padding_right;
      ps.pref_w += e->padding_left + e->padding_right;
    }

    const int skin_intrinsic_h = e->GetIntrinsicHeight();
    if (e->GetPrefHeight() != kSkinValueNotSpecified) {
      ps.pref_h = e->GetPrefHeight();
    } else if (ps.pref_h == 0 && skin_intrinsic_h != kSkinValueNotSpecified) {
      ps.pref_h = skin_intrinsic_h;
    } else {
      // Grow by padding to get the preferred size from preferred content size.
      ps.min_h += e->padding_top + e->padding_bottom;
      ps.pref_h += e->padding_top + e->padding_bottom;
    }

    if (e->GetMinWidth() != kSkinValueNotSpecified) {
      ps.min_w = e->GetMinWidth();
    } else {
      ps.min_w = std::max(ps.min_w, e->GetIntrinsicMinWidth());
    }

    if (e->GetMinHeight() != kSkinValueNotSpecified) {
      ps.min_h = e->GetMinHeight();
    } else {
      ps.min_h = std::max(ps.min_h, e->GetIntrinsicMinHeight());
    }

    if (e->GetMaxWidth() != kSkinValueNotSpecified) {
      ps.max_w = e->GetMaxWidth();
    } else {
      ps.max_w += e->padding_left + e->padding_right;
    }

    if (e->GetMaxHeight() != kSkinValueNotSpecified) {
      ps.max_h = e->GetMaxHeight();
    } else {
      ps.max_h += e->padding_top + e->padding_bottom;
    }

    // Sanitize results.
    ps.pref_w = std::max(ps.pref_w, ps.min_w);
    ps.pref_h = std::max(ps.pref_h, ps.min_h);
  }
  return ps;
}

PreferredSize Widget::GetPreferredSize(const SizeConstraints& in_constraints) {
  SizeConstraints constraints(in_constraints);
  if (m_layout_params) {
    constraints = constraints.ConstrainByLayoutParams(*m_layout_params);
  }

  // Returned cached result if valid and the constraints are the same.
  if (m_packed.is_cached_ps_valid) {
    if (m_cached_sc == constraints ||
        m_cached_ps.size_dependency == SizeDependency::kNone /*||
			// FIX: These optimizations would probably be good. Keeping
			//      disabled for now because it needs testing.
			// If *only* width depend on height, only the height matter
			(m_cached_ps.size_dependency == SizeDependency::kWidthOnHeight &&
			m_cached_sc.available_h == constraints.available_h) ||
			// If *only* height depend on width, only the width matter
			(m_cached_ps.size_dependency == SizeDependency::kHeightOnWidth &&
			m_cached_sc.available_w == constraints.available_w)*/) {
      return m_cached_ps;
    }
  }

  // Measure and save to cache.
  TB_IF_DEBUG_SETTING(Setting::kLayoutSizing,
                      last_measure_time = TBSystem::GetTimeMS());
  m_packed.is_cached_ps_valid = 1;
  m_cached_ps = OnCalculatePreferredSize(constraints);
  m_cached_sc = constraints;

  // Override the calculated ps with any specified layout parameter.
  if (m_layout_params) {
#define LP_OVERRIDE(param)                                  \
  if (m_layout_params->param != LayoutParams::kUnspecified) \
    m_cached_ps.param = m_layout_params->param;
    LP_OVERRIDE(min_w);
    LP_OVERRIDE(min_h);
    LP_OVERRIDE(max_w);
    LP_OVERRIDE(max_h);
    LP_OVERRIDE(pref_w);
    LP_OVERRIDE(pref_h);
#undef LP_OVERRIDE
    // Sanitize results.
    m_cached_ps.max_w = std::max(m_cached_ps.max_w, m_cached_ps.min_w);
    m_cached_ps.max_h = std::max(m_cached_ps.max_h, m_cached_ps.min_h);
    m_cached_ps.pref_w = std::max(m_cached_ps.pref_w, m_cached_ps.min_w);
    m_cached_ps.pref_h = std::max(m_cached_ps.pref_h, m_cached_ps.min_h);
  }
  return m_cached_ps;
}

void Widget::SetLayoutParams(const LayoutParams& lp) {
  if (!m_layout_params) {
    m_layout_params = new LayoutParams();
  }
  *m_layout_params = lp;
  m_packed.is_cached_ps_valid = 0;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Widget::InvalidateLayout(InvalidationMode il) {
  m_packed.is_cached_ps_valid = 0;
  if (GetVisibility() == Visibility::kGone) {
    return;
  }
  Invalidate();
  if (il == InvalidationMode::kRecursive && m_parent) {
    m_parent->InvalidateLayout(il);
  }
}

void Widget::InvokeProcess() {
  InvokeSkinUpdatesInternal(false);
  InvokeProcessInternal();
}

void Widget::InvokeSkinUpdatesInternal(bool force_update) {
  if (!update_skin_states && !force_update) {
    return;
  }
  update_skin_states = false;

  // Check if the skin we get is different from what we expect. That might
  // happen if the skin has some strong override dependant a condition that has
  // changed.
  // If that happens, call OnSkinChanged so the widget can react to that, and
  // invalidate layout to apply new skin properties.
  if (SkinElement* skin_elm = GetSkinBgElement()) {
    if (skin_elm->id != m_skin_bg_expected) {
      OnSkinChanged();
      m_skin_bg_expected = skin_elm->id;
      InvalidateLayout(InvalidationMode::kRecursive);
    }
  }

  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    child->InvokeSkinUpdatesInternal(true);
  }
}

void Widget::InvokeProcessInternal() {
  OnProcess();

  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    child->InvokeProcessInternal();
  }

  OnProcessAfterChildren();
}

void Widget::InvokeProcessStates(bool force_update) {
  if (!update_widget_states && !force_update) {
    return;
  }
  update_widget_states = false;

  OnProcessStates();

  for (Widget* child = GetFirstChild(); child; child = child->GetNext())
    child->InvokeProcessStates(true);
}

float Widget::CalculateOpacityInternal(SkinState state,
                                       SkinElement* skin_element) const {
  float opacity = m_opacity;
  if (skin_element) {
    opacity *= skin_element->opacity;
  }
  if (any(state & SkinState::kDisabled)) {
    opacity *= g_tb_skin->GetDefaultDisabledOpacity();
  }
  return opacity;
}

void Widget::InvokePaint(const PaintProps& parent_paint_props) {
  // Don't paint invisible widgets
  if (m_opacity == 0 || m_rect.IsEmpty() ||
      GetVisibility() != Visibility::kVisible) {
    return;
  }

  SkinState state = GetAutoState();
  SkinElement* skin_element = GetSkinBgElement();

  // Multiply current opacity with widget opacity, skin opacity and state
  // opacity.
  float old_opacity = g_renderer->GetOpacity();
  float opacity = old_opacity * CalculateOpacityInternal(state, skin_element);
  if (opacity == 0) return;

  // FIX: This does not give the correct result! Must use a new render target!
  g_renderer->SetOpacity(opacity);

  int trns_x = m_rect.x, trns_y = m_rect.y;
  g_renderer->Translate(trns_x, trns_y);

  // Paint background skin.
  Rect local_rect(0, 0, m_rect.w, m_rect.h);
  WidgetSkinConditionContext context(this);
  SkinElement* used_element = g_tb_skin->PaintSkin(
      local_rect, skin_element, static_cast<SkinState>(state), context);
  assert(!!used_element == !!skin_element);

  TB_IF_DEBUG_SETTING(
      Setting::kLayoutBounds,
      g_renderer->DrawRect(local_rect, Color(255, 255, 255, 50)));

  // Inherit properties from parent if not specified in the used skin for this
  // widget.
  PaintProps paint_props = parent_paint_props;
  if (used_element && used_element->text_color != 0) {
    paint_props.text_color = used_element->text_color;
  }

  // Paint content.
  OnPaint(paint_props);

  if (used_element) {
    g_renderer->Translate(used_element->content_ofs_x,
                          used_element->content_ofs_y);
  }

  // Paint children.
  OnPaintChildren(paint_props);

#ifdef TB_RUNTIME_DEBUG_INFO
  if (TB_DEBUG_SETTING(Setting::kLayoutSizing)) {
    // Layout debug painting. Paint recently layouted widgets with red and
    // recently measured widgets with yellow.
    // Invalidate to keep repainting until we've timed out (so it's removed).
    const uint64_t debug_time = 300;
    const uint64_t now = TBSystem::GetTimeMS();
    if (now < last_layout_time + debug_time) {
      g_renderer->DrawRect(local_rect, Color(255, 30, 30, 200));
      Invalidate();
    }
    if (now < last_measure_time + debug_time) {
      g_renderer->DrawRect(local_rect.Shrink(1, 1), Color(255, 255, 30, 200));
      Invalidate();
    }
  }
#endif  // TB_RUNTIME_DEBUG_INFO

  if (used_element) {
    g_renderer->Translate(-used_element->content_ofs_x,
                          -used_element->content_ofs_y);
  }

  g_renderer->Translate(-trns_x, -trns_y);
  g_renderer->SetOpacity(old_opacity);
}

bool Widget::InvokeEvent(WidgetEvent& ev) {
  ev.target = this;

  // First call the global listener about this event.
  // Who knows, maybe some listener will block the event or cause us
  // to be deleted.
  WeakWidgetPointer this_widget(this);
  if (WidgetListener::InvokeWidgetInvokeEvent(this, ev)) return true;

  if (!this_widget.Get()) {
    return true;  // We got removed so we actually handled this event.
  }

  if (ev.type == EventType::kChanged) {
    InvalidateSkinStates();
    m_connection.SyncFromWidget(this);
  }

  if (!this_widget.Get()) {
    return true;  // We got removed so we actually handled this event.
  }

  // Always update states after some event types.
  switch (ev.type) {
    case EventType::kClick:
    case EventType::kLongClick:
    case EventType::kChanged:
    case EventType::kKeyDown:
    case EventType::kKeyUp:
      InvalidateStates();
      break;
    default:
      break;
  };

  // Call OnEvent on this widgets and travel up through its parents if not
  // handled.
  bool handled = false;
  Widget* tmp = this;
  while (tmp && !(handled = tmp->OnEvent(ev))) {
    tmp = tmp->GetEventDestination();
  }
  return handled;
}

void Widget::StartLongClickTimer(bool touch) {
  StopLongClickTimer();
  m_long_click_timer = new LongClickTimer(this, touch);
}

void Widget::StopLongClickTimer() {
  if (!m_long_click_timer) return;
  delete m_long_click_timer;
  m_long_click_timer = nullptr;
}

bool Widget::InvokePointerDown(int x, int y, int click_count,
                               ModifierKeys modifierkeys, bool touch) {
  // If we have a captured widget then the pointer event was handled since focus
  // is changed here.
  if (!captured_widget) {
    SetCapturedWidget(GetWidgetAt(x, y, true));
    SetHoveredWidget(captured_widget, touch);
    // captured_button = button;

    // Hide focus when we use the pointer, if it's not on the focused widget.
    if (focused_widget != captured_widget) SetAutoFocusState(false);

    // Start long click timer. Only for touch events for now.
    if (touch && captured_widget && captured_widget->GetWantLongClick()) {
      captured_widget->StartLongClickTimer(touch);
    }

    // Get the closest parent window and bring it to the top.
    Window* window =
        captured_widget ? captured_widget->GetParentWindow() : nullptr;
    if (window) {
      window->Activate();
    }
  }
  if (captured_widget) {
    // Check if there's any started scroller that should be stopped.
    Widget* tmp = captured_widget;
    while (tmp) {
      if (tmp->m_scroller && tmp->m_scroller->IsStarted()) {
        // When we touch down to stop a scroller, we don't
        // want the touch to end up causing a click.
        cancel_click = true;
        tmp->m_scroller->Stop();
        break;
      }
      tmp = tmp->GetParent();
    }

    // Focus the captured widget or the closest focusable parent if it isn't
    // focusable.
    Widget* focus_target = captured_widget;
    while (focus_target) {
      if (focus_target->SetFocus(FocusReason::kPointer)) {
        break;
      }
      focus_target = focus_target->m_parent;
    }
  }
  if (captured_widget) {
    captured_widget->ConvertFromRoot(x, y);
    pointer_move_widget_x = pointer_down_widget_x = x;
    pointer_move_widget_y = pointer_down_widget_y = y;
    WidgetEvent ev(EventType::kPointerDown, x, y, touch, modifierkeys);
    ev.count = click_count;
    captured_widget->InvokeEvent(ev);
    return true;
  }

  return false;
}

bool Widget::InvokePointerUp(int x, int y, ModifierKeys modifierkeys,
                             bool touch) {
  // If we have a captured widget then we have a focused widget so the pointer
  // up event was handled
  if (captured_widget) {
    captured_widget->ConvertFromRoot(x, y);
    WidgetEvent ev_up(EventType::kPointerUp, x, y, touch, modifierkeys);
    WidgetEvent ev_click(EventType::kClick, x, y, touch, modifierkeys);
    captured_widget->InvokeEvent(ev_up);
    if (!cancel_click && captured_widget &&
        captured_widget->GetHitStatus(x, y) != HitStatus::kNoHit) {
      captured_widget->InvokeEvent(ev_click);
    }
    if (captured_widget) {  // && button == captured_button
      captured_widget->ReleaseCapture();
    }
    return true;
  }

  return false;
}

void Widget::MaybeInvokeLongClickOrContextMenu(bool touch) {
  StopLongClickTimer();
  if (captured_widget == this && !cancel_click &&
      captured_widget->GetHitStatus(
          pointer_move_widget_x, pointer_move_widget_y) != HitStatus::kNoHit) {
    // Invoke long click.
    WidgetEvent ev_long_click(EventType::kLongClick, pointer_move_widget_x,
                              pointer_move_widget_y, touch,
                              ModifierKeys::kNone);
    bool handled = captured_widget->InvokeEvent(ev_long_click);
    if (!handled) {
      // Long click not handled so invoke a context menu event instead.
      WidgetEvent ev_context_menu(EventType::kContextMenu,
                                  pointer_move_widget_x, pointer_move_widget_y,
                                  touch, ModifierKeys::kNone);
      handled = captured_widget->InvokeEvent(ev_context_menu);
    }
    // If any event was handled, suppress click when releasing pointer.
    if (handled) {
      cancel_click = true;
    }
  }
}

void Widget::InvokePointerMove(int x, int y, ModifierKeys modifierkeys,
                               bool touch) {
  SetHoveredWidget(GetWidgetAt(x, y, true), touch);
  Widget* target = captured_widget ? captured_widget : hovered_widget;

  if (target) {
    target->ConvertFromRoot(x, y);
    pointer_move_widget_x = x;
    pointer_move_widget_y = y;

    WidgetEvent ev(EventType::kPointerMove, x, y, touch, modifierkeys);
    if (target->InvokeEvent(ev)) {
      return;
    }
    // The move event was not handled, so handle panning of scrollable widgets.
    HandlePanningOnMove(x, y);
  }
}

void Widget::HandlePanningOnMove(int x, int y) {
  if (!captured_widget) return;

  // Check pointer movement.
  const int dx = pointer_down_widget_x - x;
  const int dy = pointer_down_widget_y - y;
  const int threshold = TBSystem::GetPanThreshold();
  const bool maybe_start_panning_x = std::abs(dx) >= threshold;
  const bool maybe_start_panning_y = std::abs(dy) >= threshold;

  // Do panning, or attempt starting panning (we don't know if any widget is
  // scrollable yet).
  if (captured_widget->m_packed.is_panning || maybe_start_panning_x ||
      maybe_start_panning_y) {
    // The threshold is met for not invoking any long click.
    captured_widget->StopLongClickTimer();

    int start_compensation_x = 0, start_compensation_y = 0;
    if (!captured_widget->m_packed.is_panning) {
      // When we start panning, deduct the extra distance caused by the
      // start threshold from the delta so we don't start with a sudden jump.
      int extra = threshold - 1;
      if (maybe_start_panning_x) {
        start_compensation_x = dx < 0 ? extra : -extra;
      }
      if (maybe_start_panning_y) {
        start_compensation_y = dy < 0 ? extra : -extra;
      }
    }

    // Get any active scroller and feed it with pan actions.
    Scroller* scroller = captured_widget->GetReadyScroller(dx != 0, dy != 0);
    if (!scroller) {
      return;
    }

    int old_translation_x = 0, old_translation_y = 0;
    captured_widget->GetScrollRoot()->GetChildTranslation(old_translation_x,
                                                          old_translation_y);

    if (scroller->OnPan(dx + start_compensation_x, dy + start_compensation_y)) {
      // Scroll delta changed, so we are now panning!
      captured_widget->m_packed.is_panning = true;
      cancel_click = true;

      // If the captured widget (or its scroll root) has panned, we have to
      // compensate the pointer down coordinates so we won't accumulate the
      // difference the following pan.
      int new_translation_x = 0, new_translation_y = 0;
      captured_widget->GetScrollRoot()->GetChildTranslation(new_translation_x,
                                                            new_translation_y);
      pointer_down_widget_x +=
          new_translation_x - old_translation_x + start_compensation_x;
      pointer_down_widget_y +=
          new_translation_y - old_translation_y + start_compensation_y;
    }
  }
}

bool Widget::InvokeWheel(int x, int y, int delta_x, int delta_y,
                         ModifierKeys modifierkeys) {
  SetHoveredWidget(GetWidgetAt(x, y, true), true);

  // If we have a target then the wheel event should be consumed.
  Widget* target = captured_widget ? captured_widget : hovered_widget;
  if (!target) {
    return false;
  }
  target->ConvertFromRoot(x, y);
  pointer_move_widget_x = x;
  pointer_move_widget_y = y;
  WidgetEvent ev(EventType::kWheel, x, y, true, modifierkeys);
  ev.delta_x = delta_x;
  ev.delta_y = delta_y;
  target->InvokeEvent(ev);
  return true;
}

bool Widget::InvokeKey(int key, SpecialKey special_key,
                       ModifierKeys modifierkeys, bool down) {
  bool handled = false;
  if (focused_widget) {
    // Emulate a click on the focused widget when pressing space or enter.
    if (!any(modifierkeys) && focused_widget->GetClickByKey() &&
        !focused_widget->GetDisabled() && !focused_widget->GetIsDying() &&
        (special_key == SpecialKey::kEnter || key == ' ')) {
      // Set the pressed state while the key is down, if it didn't already have
      // the pressed state.
      static bool check_pressed_state = true;
      static bool had_pressed_state = false;
      if (down && check_pressed_state) {
        had_pressed_state = focused_widget->GetState(SkinState::kPressed);
        check_pressed_state = false;
      }
      if (!down) {
        check_pressed_state = true;
      }

      if (!had_pressed_state) {
        focused_widget->SetState(SkinState::kPressed, down);
        focused_widget->m_packed.has_key_pressed_state = down;
      }

      // Invoke the click event.
      if (!down) {
        WidgetEvent ev(EventType::kClick, m_rect.w / 2, m_rect.h / 2, true);
        focused_widget->InvokeEvent(ev);
      }
      handled = true;
    } else {
      // Invoke the key event on the focused widget.
      WidgetEvent ev(down ? EventType::kKeyDown : EventType::kKeyUp);
      ev.key = key;
      ev.special_key = special_key;
      ev.modifierkeys = modifierkeys;
      handled = focused_widget->InvokeEvent(ev);
    }
  }

  // Move focus between widgets.
  if (down && !handled && special_key == SpecialKey::kTab) {
    handled = MoveFocus(!any(modifierkeys & ModifierKeys::kShift));

    // Show the focus when we move it by keyboard.
    if (handled) {
      SetAutoFocusState(true);
    }
  }
  return handled;
}

void Widget::ReleaseCapture() {
  if (this == captured_widget) {
    SetCapturedWidget(nullptr);
  }
}

void Widget::ConvertToRoot(int& x, int& y) const {
  const Widget* tmp = this;
  while (tmp->m_parent) {
    x += tmp->m_rect.x;
    y += tmp->m_rect.y;
    tmp = tmp->m_parent;

    if (tmp) {
      int child_translation_x, child_translation_y;
      tmp->GetChildTranslation(child_translation_x, child_translation_y);
      x += child_translation_x;
      y += child_translation_y;
    }
  }
}

void Widget::ConvertFromRoot(int& x, int& y) const {
  const Widget* tmp = this;
  while (tmp->m_parent) {
    x -= tmp->m_rect.x;
    y -= tmp->m_rect.y;
    tmp = tmp->m_parent;

    if (tmp) {
      int child_translation_x, child_translation_y;
      tmp->GetChildTranslation(child_translation_x, child_translation_y);
      x -= child_translation_x;
      y -= child_translation_y;
    }
  }
}

// static
void Widget::SetHoveredWidget(Widget* widget, bool touch) {
  if (Widget::hovered_widget == widget) {
    return;
  }
  if (widget && widget->GetState(SkinState::kDisabled)) {
    return;
  }

  // We may apply hover state automatically so the widget might need to be
  // updated.
  if (Widget::hovered_widget) {
    Widget::hovered_widget->Invalidate();
    Widget::hovered_widget->InvalidateSkinStates();
  }

  Widget::hovered_widget = widget;

  if (Widget::hovered_widget) {
    Widget::hovered_widget->Invalidate();
    Widget::hovered_widget->InvalidateSkinStates();

    // Cursor based movement should set hover state automatically, but touch
    // events should not (since touch doesn't really move unless pressed).
    Widget::hovered_widget->m_packed.no_automatic_hover_state = touch;
  }
}

// static
void Widget::SetCapturedWidget(Widget* widget) {
  if (Widget::captured_widget == widget) {
    return;
  }
  if (widget && widget->GetState(SkinState::kDisabled)) {
    return;
  }

  if (Widget::captured_widget) {
    // Stop panning when capture change (most likely changing to nullptr because
    // of InvokePointerUp).
    // Notify any active scroller so it may begin scrolling.
    if (Scroller* scroller = Widget::captured_widget->FindStartedScroller()) {
      if (Widget::captured_widget->m_packed.is_panning) {
        scroller->OnPanReleased();
      } else {
        scroller->Stop();
      }
    }
    Widget::captured_widget->m_packed.is_panning = false;

    // We apply pressed state automatically so the widget might need to be
    // updated.
    Widget::captured_widget->Invalidate();
    Widget::captured_widget->InvalidateSkinStates();

    Widget::captured_widget->StopLongClickTimer();
  }
  cancel_click = false;

  Widget* old_capture = Widget::captured_widget;

  Widget::captured_widget = widget;

  if (old_capture) old_capture->OnCaptureChanged(false);

  if (Widget::captured_widget) {
    Widget::captured_widget->Invalidate();
    Widget::captured_widget->InvalidateSkinStates();
    Widget::captured_widget->OnCaptureChanged(true);
  }
}

bool Widget::SetFontDescription(const FontDescription& font_desc) {
  if (m_font_desc == font_desc) return true;

  // Set the font description only if we have a matching font, or succeed
  // creating one.
  if (g_font_manager->HasFontFace(font_desc)) {
    m_font_desc = font_desc;
  } else if (g_font_manager->CreateFontFace(font_desc)) {
    m_font_desc = font_desc;
  } else {
    return false;
  }

  InvokeFontChanged();
  return true;
}

void Widget::InvokeFontChanged() {
  OnFontChanged();

  // Recurse to children that inherit the font.
  for (Widget* child = GetFirstChild(); child; child = child->GetNext()) {
    if (child->m_font_desc.GetFontFaceID() == 0) {
      child->InvokeFontChanged();
    }
  }
}

FontDescription Widget::GetCalculatedFontDescription() const {
  const Widget* tmp = this;
  while (tmp) {
    if (tmp->m_font_desc.GetFontFaceID() != 0) {
      return tmp->m_font_desc;
    }
    tmp = tmp->m_parent;
  }
  return g_font_manager->GetDefaultFontDescription();
}

FontFace* Widget::GetFont() const {
  return g_font_manager->GetFontFace(GetCalculatedFontDescription());
}

}  // namespace tb
