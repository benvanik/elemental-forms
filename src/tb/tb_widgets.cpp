/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_widgets.h"

#include <algorithm>
#include <cassert>

#include "tb_scroller.h"
#include "tb_select_item.h"
#include "tb_text_box.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_window.h"

#include "tb/graphics/renderer.h"
#include "tb/parsing/element_inflater.h"
#include "tb/parsing/parse_node.h"
#include "tb/resources/font_manager.h"
#include "tb/util/debug.h"
#include "tb/util/math.h"
#include "tb/util/metrics.h"
#include "tb/value.h"

namespace tb {

using graphics::Renderer;

Element* Element::hovered_element = nullptr;
Element* Element::captured_element = nullptr;
Element* Element::focused_element = nullptr;
int Element::pointer_down_element_x = 0;
int Element::pointer_down_element_y = 0;
int Element::pointer_move_element_x = 0;
int Element::pointer_move_element_y = 0;
bool Element::cancel_click = false;
bool Element::update_element_states = true;
bool Element::update_skin_states = true;
bool Element::show_focus_state = false;

// One shot timer for long click event.
class LongClickTimer : private MessageHandler {
 public:
  LongClickTimer(Element* element, bool touch)
      : m_element(element), m_touch(touch) {
    PostMessageDelayed(TBIDC("LongClickTimer"), nullptr,
                       util::GetLongClickDelayMS());
  }
  void OnMessageReceived(Message* msg) override {
    assert(msg->message == TBIDC("LongClickTimer"));
    m_element->MaybeInvokeLongClickOrContextMenu(m_touch);
  }

 private:
  Element* m_element;
  bool m_touch;
};

Element::PaintProps::PaintProps() {
  // Set the default properties, used for the root elements
  // calling InvokePaint. The base values for all inheritance.
  text_color = resources::Skin::get()->GetDefaultTextColor();
}

void Element::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(Element, Value::Type::kNull, ElementZ::kTop);
}

Element::Element() = default;

Element::~Element() {
  assert(!m_parent);  // A element must be removed from parent before deleted.
  m_packed.is_dying = true;

  if (this == hovered_element) {
    hovered_element = nullptr;
  }
  if (this == captured_element) {
    captured_element = nullptr;
  }
  if (this == focused_element) {
    focused_element = nullptr;
  }

  ElementListener::InvokeElementDelete(this);
  DeleteAllChildren();

  delete m_scroller;
  delete m_layout_params;

  StopLongClickTimer();

  assert(!m_listeners
              .HasLinks());  // There's still listeners added to this element!
}

bool Element::LoadFile(const char* filename) {
  return parsing::ElementFactory::get()->LoadFile(this, filename);
}

bool Element::LoadData(const char* data, size_t data_length) {
  return parsing::ElementFactory::get()->LoadData(this, data, data_length);
}

void Element::LoadNodeTree(parsing::ParseNode* node) {
  return parsing::ElementFactory::get()->LoadNodeTree(this, node);
}

// Sets the id from the given node.
void Element::SetIdFromNode(TBID& id, parsing::ParseNode* node) {
  if (!node) return;
  if (node->GetValue().is_string()) {
    id.reset(node->GetValue().as_string());
  } else {
    id.reset(node->GetValue().as_integer());
  }
}

void Element::ReadItemNodes(parsing::ParseNode* parent_node,
                            GenericStringItemSource* target_source) {
  // If there is a items node, loop through all its children and add
  // items to the target item source.
  auto items_node = parent_node->GetNode("items");
  if (!items_node) {
    return;
  }
  for (auto node = items_node->GetFirstChild(); node; node = node->GetNext()) {
    if (std::strcmp(node->GetName(), "item") != 0) {
      continue;
    }
    const char* text = node->GetValueString("text", "");
    TBID item_id;
    if (auto id_node = node->GetNode("id")) {
      Element::SetIdFromNode(item_id, id_node);
    }
    auto item = std::make_unique<GenericStringItem>(text, item_id);
    target_source->AddItem(std::move(item));
  }
}

void Element::OnInflate(const parsing::InflateInfo& info) {
  Element::SetIdFromNode(id(), info.node->GetNode("id"));
  Element::SetIdFromNode(GetGroupID(), info.node->GetNode("group-id"));

  if (info.sync_type == Value::Type::kFloat) {
    SetValueDouble(info.node->GetValueFloat("value", 0));
  } else {
    SetValue(info.node->GetValueInt("value", 0));
  }

  if (auto data_node = info.node->GetNode("data")) {
    data.Copy(data_node->GetValue());
  }

  SetIsGroupRoot(
      info.node->GetValueInt("is-group-root", GetIsGroupRoot()) ? true : false);

  SetIsFocusable(
      info.node->GetValueInt("is-focusable", GetIsFocusable()) ? true : false);

  SetWantLongClick(info.node->GetValueInt("want-long-click", GetWantLongClick())
                       ? true
                       : false);

  SetIgnoreInput(
      info.node->GetValueInt("ignore-input", GetIgnoreInput()) ? true : false);

  SetOpacity(info.node->GetValueFloat("opacity", GetOpacity()));

  if (const char* text = info.node->GetValueString("text", nullptr)) {
    SetText(text);
  }

  if (const char* connection =
          info.node->GetValueStringRaw("connection", nullptr)) {
    // If we already have a element value with this name, just connect to it and
    // the element will adjust its value to it. Otherwise create a new element
    // value, and give it the value we got from the resource.
    if (ElementValue* value = ValueGroup::get()->GetValue(connection)) {
      Connect(value);
    } else if (ElementValue* value = ValueGroup::get()->CreateValueIfNeeded(
                   connection, info.sync_type)) {
      value->SetFromElement(this);
      Connect(value);
    }
  }
  if (const char* axis = info.node->GetValueString("axis", nullptr)) {
    SetAxis(tb::from_string(axis, Axis::kY));
  }
  if (const char* gravity = info.node->GetValueString("gravity", nullptr)) {
    Gravity g = Gravity::kNone;
    if (strstr(gravity, "left")) g |= Gravity::kLeft;
    if (strstr(gravity, "top")) g |= Gravity::kTop;
    if (strstr(gravity, "right")) g |= Gravity::kRight;
    if (strstr(gravity, "bottom")) g |= Gravity::kBottom;
    if (strstr(gravity, "all")) g |= Gravity::kAll;
    if (!any(g & Gravity::kLeftRight)) g |= Gravity::kLeft;
    if (!any(g & Gravity::kTopBottom)) g |= Gravity::kTop;
    SetGravity(g);
  }
  if (const char* visibility =
          info.node->GetValueString("visibility", nullptr)) {
    SetVisibilility(from_string(visibility, GetVisibility()));
  }
  if (const char* state = info.node->GetValueString("state", nullptr)) {
    if (strstr(state, "disabled")) {
      SetState(Element::State::kDisabled, true);
    }
  }
  if (const char* skin = info.node->GetValueString("skin", nullptr)) {
    SetSkinBg(skin);
  }
  if (auto lp = info.node->GetNode("lp")) {
    LayoutParams layout_params;
    if (GetLayoutParams()) {
      layout_params = *GetLayoutParams();
    }
    auto dc = resources::Skin::get()->GetDimensionConverter();
    if (const char* str = lp->GetValueString("width", nullptr)) {
      layout_params.set_width(
          dc->GetPxFromString(str, LayoutParams::kUnspecified));
    }
    if (const char* str = lp->GetValueString("height", nullptr)) {
      layout_params.set_height(
          dc->GetPxFromString(str, LayoutParams::kUnspecified));
    }
    if (const char* str = lp->GetValueString("min-width", nullptr)) {
      layout_params.min_w =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("max-width", nullptr)) {
      layout_params.max_w =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("pref-width", nullptr)) {
      layout_params.pref_w =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("min-height", nullptr)) {
      layout_params.min_h =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("max-height", nullptr)) {
      layout_params.max_h =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    if (const char* str = lp->GetValueString("pref-height", nullptr)) {
      layout_params.pref_h =
          dc->GetPxFromString(str, LayoutParams::kUnspecified);
    }
    SetLayoutParams(layout_params);
  }

  SetTooltip(info.node->GetValueString("tooltip", nullptr));

  // Add the new element to the hiearchy if not already added.
  if (!GetParent()) info.target->AddChild(this, info.target->GetZInflate());

  // Read the font now when the element is in the hiearchy so inheritance works.
  if (auto font = info.node->GetNode("font")) {
    FontDescription fd = GetCalculatedFontDescription();
    if (const char* size = font->GetValueString("size", nullptr)) {
      int new_size =
          resources::Skin::get()->GetDimensionConverter()->GetPxFromString(
              size, fd.GetSize());
      fd.SetSize(new_size);
    }
    if (const char* name = font->GetValueString("name", nullptr)) {
      fd.set_id(name);
    }
    SetFontDescription(fd);
  }

  info.target->OnInflateChild(this);

  if (auto rect_node = info.node->GetNode("rect")) {
    auto dc = resources::Skin::get()->GetDimensionConverter();
    Value& val = rect_node->GetValue();
    if (val.array_size() == 4) {
      set_rect({dc->GetPxFromValue(val.as_array()->at(0), 0),
                dc->GetPxFromValue(val.as_array()->at(1), 0),
                dc->GetPxFromValue(val.as_array()->at(2), 0),
                dc->GetPxFromValue(val.as_array()->at(3), 0)});
    }
  }
}

void Element::set_rect(const Rect& rect) {
  if (m_rect.equals(rect)) {
    return;
  }

  Rect old_rect = m_rect;
  m_rect = rect;
  if (old_rect.w != m_rect.w || old_rect.h != m_rect.h) {
    OnResized(old_rect.w, old_rect.h);
  }

  Invalidate();
}

void Element::Invalidate() {
  if (!GetVisibilityCombined() && !m_rect.empty()) {
    return;
  }
  Element* tmp = this;
  while (tmp) {
    tmp->OnInvalid();
    tmp = tmp->m_parent;
  }
}

void Element::InvalidateStates() {
  update_element_states = true;
  InvalidateSkinStates();
}

void Element::InvalidateSkinStates() { update_skin_states = true; }

void Element::Die() {
  if (m_packed.is_dying) {
    return;
  }
  m_packed.is_dying = true;
  OnDie();
  if (!ElementListener::InvokeElementDying(this)) {
    // No one was interested, so die immediately.
    if (m_parent) {
      m_parent->RemoveChild(this);
    }
    delete this;
  }
}

Element* Element::GetElementByIDInternal(const TBID& id,
                                         const util::tb_type_id_t type_id) {
  if (m_id == id && (!type_id || IsOfTypeId(type_id))) {
    return this;
  }
  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    if (Element* sub_child = child->GetElementByIDInternal(id, type_id)) {
      return sub_child;
    }
  }
  return nullptr;
}

std::string Element::GetTextByID(const TBID& id) {
  if (Element* element = GetElementByID(id)) {
    return element->GetText();
  }
  return "";
}

int Element::GetValueByID(const TBID& id) {
  if (Element* element = GetElementByID(id)) {
    return element->GetValue();
  }
  return 0;
}

void Element::set_id(const TBID& id) {
  m_id = id;
  InvalidateSkinStates();
}

void Element::SetStateRaw(Element::State state) {
  if (m_state == state) return;
  m_state = state;
  Invalidate();
  InvalidateSkinStates();
}

void Element::SetState(Element::State state, bool on) {
  SetStateRaw(on ? m_state | state : m_state & ~state);
}

Element::State Element::GetAutoState() const {
  Element::State state = m_state;
  bool add_pressed_state =
      !cancel_click && this == captured_element && this == hovered_element;
  if (add_pressed_state) {
    state |= Element::State::kPressed;
  }
  if (this == hovered_element &&
      (!m_packed.no_automatic_hover_state || add_pressed_state)) {
    state |= Element::State::kHovered;
  }
  if (this == focused_element && show_focus_state) {
    state |= Element::State::kFocused;
  }
#ifdef TB_ALWAYS_SHOW_EDIT_FOCUS
  else if (this == focused_element && IsOfType<TextBox>()) {
    state |= Element::State::kFocused;
  }
#endif  // TB_ALWAYS_SHOW_EDIT_FOCUS
  return state;
}

// static
void Element::SetAutoFocusState(bool on) {
  if (show_focus_state == on) {
    return;
  }
  show_focus_state = on;
  if (focused_element) {
    focused_element->Invalidate();
  }
}

void Element::SetOpacity(float opacity) {
  opacity = util::Clamp(opacity, 0.f, 1.f);
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

void Element::SetVisibilility(Visibility vis) {
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

Visibility Element::GetVisibility() const {
  return static_cast<Visibility>(m_packed.visibility);
}

bool Element::GetVisibilityCombined() const {
  const Element* tmp = this;
  while (tmp) {
    if (tmp->GetOpacity() == 0 ||
        tmp->GetVisibility() != Visibility::kVisible) {
      return false;
    }
    tmp = tmp->m_parent;
  }
  return true;
}

bool Element::GetDisabled() const {
  const Element* tmp = this;
  while (tmp) {
    if (tmp->GetState(Element::State::kDisabled)) {
      return true;
    }
    tmp = tmp->m_parent;
  }
  return false;
}

void Element::AddChild(Element* child, ElementZ z, InvokeInfo info) {
  AddChildRelative(
      child, z == ElementZ::kTop ? ElementZRel::kAfter : ElementZRel::kBefore,
      nullptr, info);
}

void Element::AddChildRelative(Element* child, ElementZRel z,
                               Element* reference, InvokeInfo info) {
  assert(!child->m_parent);
  child->m_parent = this;

  if (reference) {
    if (z == ElementZRel::kBefore) {
      m_children.AddBefore(child, reference);
    } else {
      m_children.AddAfter(child, reference);
    }
  } else {
    // If there is no reference element, before means first and after means
    // last.
    if (z == ElementZRel::kBefore) {
      m_children.AddFirst(child);
    } else {
      m_children.AddLast(child);
    }
  }

  if (info == InvokeInfo::kNormal) {
    OnChildAdded(child);
    child->OnAdded();
    ElementListener::InvokeElementAdded(this, child);
  }
  InvalidateLayout(InvalidationMode::kRecursive);
  Invalidate();
  InvalidateSkinStates();
}

void Element::RemoveChild(Element* child, InvokeInfo info) {
  assert(child->m_parent);

  if (info == InvokeInfo::kNormal) {
    // If we're not being deleted and delete the focused element, try
    // to keep the focus in this element by moving it to the next element.
    if (!m_packed.is_dying && child == focused_element) {
      m_parent->MoveFocus(true);
    }

    OnChildRemove(child);
    child->OnRemove();
    ElementListener::InvokeElementRemove(this, child);
  }

  m_children.Remove(child);
  child->m_parent = nullptr;

  InvalidateLayout(InvalidationMode::kRecursive);
  Invalidate();
  InvalidateSkinStates();
}

void Element::DeleteAllChildren() {
  while (Element* child = GetFirstChild()) {
    RemoveChild(child);
    delete child;
  }
}

void Element::SetZ(ElementZ z) {
  if (!m_parent) return;
  if (z == ElementZ::kTop && this == m_parent->m_children.GetLast()) {
    return;  // Already at the top
  }
  if (z == ElementZ::kBottom && this == m_parent->m_children.GetFirst()) {
    return;  // Already at the top
  }
  Element* parent = m_parent;
  parent->RemoveChild(this, InvokeInfo::kNoCallbacks);
  parent->AddChild(this, z, InvokeInfo::kNoCallbacks);
}

void Element::SetGravity(Gravity g) {
  if (m_gravity == g) return;
  m_gravity = g;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Element::SetSkinBg(const TBID& skin_bg, InvokeInfo info) {
  if (skin_bg == m_skin_bg) return;

  // Set the skin and m_skin_bg_expected. During InvokeProcess, we will detect
  // if any element get a different element due to conditions and strong
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

resources::SkinElement* Element::GetSkinBgElement() {
  ElementSkinConditionContext context(this);
  Element::State state = GetAutoState();
  return resources::Skin::get()->GetSkinElementStrongOverride(
      m_skin_bg, static_cast<Element::State>(state), context);
}

Element* Element::FindScrollableElement(bool scroll_x, bool scroll_y) {
  Element* candidate = this;
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

Scroller* Element::FindStartedScroller() {
  Element* candidate = this;
  while (candidate) {
    if (candidate->m_scroller && candidate->m_scroller->IsStarted()) {
      return candidate->m_scroller;
    }
    candidate = candidate->GetParent();
  }
  return nullptr;
}

Scroller* Element::GetReadyScroller(bool scroll_x, bool scroll_y) {
  if (Scroller* scroller = FindStartedScroller()) return scroller;
  // We didn't have any active scroller, so create one for the nearest
  // scrollable parent.
  if (Element* scrollable_element = FindScrollableElement(scroll_x, scroll_y)) {
    return scrollable_element->GetScroller();
  }
  return nullptr;
}

Scroller* Element::GetScroller() {
  if (!m_scroller) {
    m_scroller = new Scroller(this);
  }
  return m_scroller;
}

void Element::ScrollToSmooth(int x, int y) {
  ScrollInfo info = GetScrollInfo();
  int dx = x - info.x;
  int dy = y - info.y;
  if (Scroller* scroller = GetReadyScroller(dx != 0, dy != 0)) {
    scroller->OnScrollBy(dx, dy, false);
  }
}

void Element::ScrollBySmooth(int dx, int dy) {
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

void Element::ScrollBy(int dx, int dy) {
  ScrollInfo info = GetScrollInfo();
  ScrollTo(info.x + dx, info.y + dy);
}

void Element::ScrollByRecursive(int& dx, int& dy) {
  Element* tmp = this;
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

void Element::ScrollIntoViewRecursive() {
  Rect scroll_to_rect = m_rect;
  Element* tmp = this;
  while (tmp->m_parent) {
    tmp->m_parent->ScrollIntoView(scroll_to_rect);
    scroll_to_rect.x += tmp->m_parent->m_rect.x;
    scroll_to_rect.y += tmp->m_parent->m_rect.y;
    tmp = tmp->m_parent;
  }
}

void Element::ScrollIntoView(const Rect& rect) {
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

bool Element::SetFocus(FocusReason reason, InvokeInfo info) {
  if (focused_element == this) return true;
  if (GetDisabled() || !GetIsFocusable() || !GetVisibilityCombined() ||
      GetIsDying()) {
    return false;
  }

  // Update windows last focus.
  Window* window = GetParentWindow();
  if (window) {
    window->SetLastFocus(this);
    // If not active, just return. We should get focus when the window is
    // activated.
    // Exception for windows that doesn't activate. They may contain focusable
    // elements.
    if (!window->IsActive() &&
        any(window->GetSettings() & WindowSettings::kCanActivate)) {
      return true;
    }
  }

  if (focused_element) {
    focused_element->Invalidate();
    focused_element->InvalidateSkinStates();
  }

  WeakElementPointer old_focus(focused_element);
  focused_element = this;

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
    if (Element* old = old_focus.Get()) {
      // The currently focused element still has the pressed state set by the
      // emulated click (by keyboard), so unset it before we unfocus it so it's
      // not stuck in pressed state.
      if (old->m_packed.has_key_pressed_state) {
        old->SetState(Element::State::kPressed, false);
        old->m_packed.has_key_pressed_state = false;
      }
      old->OnFocusChanged(false);
    }
    if (old_focus.Get()) {
      ElementListener::InvokeElementFocusChanged(old_focus.Get(), false);
    }
    if (focused_element && focused_element == this) {
      focused_element->OnFocusChanged(true);
    }
    if (focused_element && focused_element == this) {
      ElementListener::InvokeElementFocusChanged(focused_element, true);
    }
  }
  return true;
}

bool Element::SetFocusRecursive(FocusReason reason) {
  // Search for a child element that accepts focus.
  Element* child = GetFirstChild();
  while (child) {
    if (child->SetFocus(FocusReason::kUnknown)) {
      return true;
    }
    child = child->GetNextDeep(this);
  }
  return false;
}

bool Element::MoveFocus(bool forward) {
  Element* origin = focused_element;
  if (!origin) {
    origin = this;
  }

  Element* root = origin->GetParentWindow();
  if (!root) {
    root = origin->GetParentRoot();
  }

  Element* current = origin;
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

Element* Element::GetNextDeep(const Element* bounding_ancestor) const {
  if (m_children.GetFirst()) {
    return GetFirstChild();
  }
  for (const Element* element = this; element != bounding_ancestor;
       element = element->m_parent) {
    if (element->next) {
      return element->GetNext();
    }
  }
  return nullptr;
}

Element* Element::GetPrevDeep() const {
  if (!prev) return m_parent;
  Element* element = GetPrev();
  while (element->m_children.GetLast()) {
    element = element->GetLastChild();
  }
  return element;
}

Element* Element::GetLastLeaf() const {
  if (Element* element = GetLastChild()) {
    while (element->GetLastChild()) {
      element = element->GetLastChild();
    }
    return element;
  }
  return nullptr;
}

bool Element::GetIsInteractable() const {
  return !(m_opacity == 0 || GetIgnoreInput() ||
           GetState(Element::State::kDisabled) || GetIsDying() ||
           GetVisibility() != Visibility::kVisible);
}

HitStatus Element::GetHitStatus(int x, int y) {
  if (!GetIsInteractable()) return HitStatus::kNoHit;
  return x >= 0 && y >= 0 && x < m_rect.w && y < m_rect.h ? HitStatus::kHit
                                                          : HitStatus::kNoHit;
}

Element* Element::GetElementAt(int x, int y, bool include_children) const {
  int child_translation_x, child_translation_y;
  GetChildTranslation(child_translation_x, child_translation_y);
  x -= child_translation_x;
  y -= child_translation_y;

  Element* tmp = GetFirstChild();
  Element* last_match = nullptr;
  while (tmp) {
    HitStatus hit_status =
        tmp->GetHitStatus(x - tmp->m_rect.x, y - tmp->m_rect.y);
    if (hit_status != HitStatus::kNoHit) {
      if (include_children && hit_status != HitStatus::kHitNoChildren) {
        last_match = tmp->GetElementAt(x - tmp->m_rect.x, y - tmp->m_rect.y,
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

Element* Element::GetChildFromIndex(int index) const {
  int i = 0;
  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    if (i++ == index) return child;
  }
  return nullptr;
}

int Element::GetIndexFromChild(Element* child) const {
  assert(child->GetParent() == this);
  int i = 0;
  for (Element* tmp = GetFirstChild(); tmp; tmp = tmp->GetNext(), ++i) {
    if (tmp == child) {
      return i;
    }
  }
  return -1;  // Should not happen!
}

bool Element::IsAncestorOf(Element* other_element) const {
  while (other_element) {
    if (other_element == this) {
      return true;
    }
    other_element = other_element->m_parent;
  }
  return false;
}

bool Element::IsEventDestinationFor(Element* other_element) const {
  while (other_element) {
    if (other_element == this) {
      return true;
    }
    other_element = other_element->GetEventDestination();
  }
  return false;
}

Element* Element::GetParentRoot() {
  Element* tmp = this;
  while (tmp->m_parent) {
    tmp = tmp->m_parent;
  }
  return tmp;
}

Window* Element::GetParentWindow() {
  Element* tmp = this;
  while (tmp && !tmp->IsOfType<Window>()) {
    tmp = tmp->m_parent;
  }
  return static_cast<Window*>(tmp);
}

void Element::AddListener(ElementListener* listener) {
  m_listeners.AddLast(listener);
}

void Element::RemoveListener(ElementListener* listener) {
  m_listeners.Remove(listener);
}

bool Element::HasListener(ElementListener* listener) const {
  return m_listeners.ContainsLink(listener);
}

void Element::OnPaintChildren(const PaintProps& paint_props) {
  if (!m_children.GetFirst()) return;

  // Translate renderer with child translation.
  int child_translation_x, child_translation_y;
  GetChildTranslation(child_translation_x, child_translation_y);
  Renderer::get()->Translate(child_translation_x, child_translation_y);

  Rect clip_rect = Renderer::get()->GetClipRect();

  // Invoke paint on all children that are in the current visible rect.
  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    if (clip_rect.intersects(child->m_rect)) {
      child->InvokePaint(paint_props);
    }
  }

  // Invoke paint of overlay elements on all children that are in the current
  // visible rect.
  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    if (clip_rect.intersects(child->m_rect) &&
        child->GetVisibility() == Visibility::kVisible) {
      auto skin_element = child->GetSkinBgElement();
      if (skin_element && skin_element->HasOverlayElements()) {
        // Update the renderer with the elements opacity.
        Element::State state = child->GetAutoState();
        float old_opacity = Renderer::get()->GetOpacity();
        float opacity =
            old_opacity * child->CalculateOpacityInternal(state, skin_element);
        if (opacity > 0) {
          Renderer::get()->SetOpacity(opacity);

          ElementSkinConditionContext context(child);
          resources::Skin::get()->PaintSkinOverlay(
              child->m_rect, skin_element, static_cast<Element::State>(state),
              context);

          Renderer::get()->SetOpacity(old_opacity);
        }
      }
    }
  }

  // Draw generic focus skin if the focused element is one of the children, and
  // the skin doesn't have a skin state for focus which would already be
  // painted.
  if (focused_element && focused_element->m_parent == this) {
    ElementSkinConditionContext context(focused_element);
    auto skin_element = focused_element->GetSkinBgElement();
    if (!skin_element ||
        !skin_element->HasState(Element::State::kFocused, context)) {
      Element::State state = focused_element->GetAutoState();
      if (any(state & Element::State::kFocused)) {
        resources::Skin::get()->PaintSkin(
            focused_element->m_rect, TBIDC("generic_focus"),
            static_cast<Element::State>(state), context);
      }
    }
  }

  Renderer::get()->Translate(-child_translation_x, -child_translation_y);
}

void Element::OnResized(int old_w, int old_h) {
  int dw = m_rect.w - old_w;
  int dh = m_rect.h - old_h;
  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
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
    child->set_rect(rect);
  }
}

void Element::OnInflateChild(Element* child) {
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
  child->set_rect(child_rect);
}

Rect Element::GetPaddingRect() {
  Rect padding_rect(0, 0, m_rect.w, m_rect.h);
  if (auto e = GetSkinBgElement()) {
    padding_rect.x += e->padding_left;
    padding_rect.y += e->padding_top;
    padding_rect.w -= e->padding_left + e->padding_right;
    padding_rect.h -= e->padding_top + e->padding_bottom;
  }
  return padding_rect;
}

PreferredSize Element::OnCalculatePreferredContentSize(
    const SizeConstraints& constraints) {
  // The default preferred size is calculated to satisfy the children
  // in the best way. Since this is the default, it's probably not a
  // layouting element and children are resized purely by gravity.

  // Allow this element a larger maximum if our gravity wants both ways,
  // otherwise don't grow more than the largest child.
  bool apply_max_w =
      !any(m_gravity & Gravity::kLeft) && any(m_gravity & Gravity::kRight);
  bool apply_max_h =
      !any(m_gravity & Gravity::kTop) && any(m_gravity & Gravity::kBottom);
  bool has_layouting_children = false;
  PreferredSize ps;

  auto bg_skin = GetSkinBgElement();
  int horizontal_padding =
      bg_skin ? bg_skin->padding_left + bg_skin->padding_right : 0;
  int vertical_padding =
      bg_skin ? bg_skin->padding_top + bg_skin->padding_bottom : 0;
  SizeConstraints inner_sc =
      constraints.ConstrainByPadding(horizontal_padding, vertical_padding);

  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
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

PreferredSize Element::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = OnCalculatePreferredContentSize(constraints);
  assert(ps.pref_w >= ps.min_w);
  assert(ps.pref_h >= ps.min_h);

  auto e = GetSkinBgElement();
  if (!e) {
    return ps;
  }

  using resources::kSkinValueNotSpecified;

  // Override the elements preferences with skin attributes that has been
  // specified.
  // If not set by the element, calculate based on the intrinsic size of the
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

  return ps;
}

PreferredSize Element::GetPreferredSize(const SizeConstraints& in_constraints) {
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
  TB_IF_DEBUG_SETTING(util::DebugInfo::Setting::kLayoutSizing,
                      last_measure_time = util::GetTimeMS());
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

void Element::SetLayoutParams(const LayoutParams& lp) {
  if (!m_layout_params) {
    m_layout_params = new LayoutParams();
  }
  *m_layout_params = lp;
  m_packed.is_cached_ps_valid = 0;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Element::InvalidateLayout(InvalidationMode il) {
  m_packed.is_cached_ps_valid = 0;
  if (GetVisibility() == Visibility::kGone) {
    return;
  }
  Invalidate();
  if (il == InvalidationMode::kRecursive && m_parent) {
    m_parent->InvalidateLayout(il);
  }
}

void Element::InvokeProcess() {
  InvokeSkinUpdatesInternal(false);
  InvokeProcessInternal();
}

void Element::InvokeSkinUpdatesInternal(bool force_update) {
  if (!update_skin_states && !force_update) {
    return;
  }
  update_skin_states = false;

  // Check if the skin we get is different from what we expect. That might
  // happen if the skin has some strong override dependant a condition that has
  // changed.
  // If that happens, call OnSkinChanged so the element can react to that, and
  // invalidate layout to apply new skin properties.
  if (auto skin_elm = GetSkinBgElement()) {
    if (skin_elm->id != m_skin_bg_expected) {
      OnSkinChanged();
      m_skin_bg_expected = skin_elm->id;
      InvalidateLayout(InvalidationMode::kRecursive);
    }
  }

  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    child->InvokeSkinUpdatesInternal(true);
  }
}

void Element::InvokeProcessInternal() {
  OnProcess();

  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    child->InvokeProcessInternal();
  }

  OnProcessAfterChildren();
}

void Element::InvokeProcessStates(bool force_update) {
  if (!update_element_states && !force_update) {
    return;
  }
  update_element_states = false;

  OnProcessStates();

  for (Element* child = GetFirstChild(); child; child = child->GetNext())
    child->InvokeProcessStates(true);
}

float Element::CalculateOpacityInternal(
    Element::State state, resources::SkinElement* skin_element) const {
  float opacity = m_opacity;
  if (skin_element) {
    opacity *= skin_element->opacity;
  }
  if (any(state & Element::State::kDisabled)) {
    opacity *= resources::Skin::get()->GetDefaultDisabledOpacity();
  }
  return opacity;
}

void Element::InvokePaint(const PaintProps& parent_paint_props) {
  // Don't paint invisible elements
  if (m_opacity == 0 || m_rect.empty() ||
      GetVisibility() != Visibility::kVisible) {
    return;
  }

  Element::State state = GetAutoState();
  auto skin_element = GetSkinBgElement();

  // Multiply current opacity with element opacity, skin opacity and state
  // opacity.
  float old_opacity = Renderer::get()->GetOpacity();
  float opacity = old_opacity * CalculateOpacityInternal(state, skin_element);
  if (opacity == 0) return;

  // FIX: This does not give the correct result! Must use a new render target!
  Renderer::get()->SetOpacity(opacity);

  int trns_x = m_rect.x, trns_y = m_rect.y;
  Renderer::get()->Translate(trns_x, trns_y);

  // Paint background skin.
  Rect local_rect(0, 0, m_rect.w, m_rect.h);
  ElementSkinConditionContext context(this);
  auto used_element = resources::Skin::get()->PaintSkin(
      local_rect, skin_element, static_cast<Element::State>(state), context);
  assert(!!used_element == !!skin_element);

  TB_IF_DEBUG_SETTING(
      util::DebugInfo::Setting::kLayoutBounds,
      Renderer::get()->DrawRect(local_rect, Color(255, 255, 255, 50)));

  // Inherit properties from parent if not specified in the used skin for this
  // element.
  PaintProps paint_props = parent_paint_props;
  if (used_element && used_element->text_color != 0) {
    paint_props.text_color = used_element->text_color;
  }

  // Paint content.
  OnPaint(paint_props);

  if (used_element) {
    Renderer::get()->Translate(used_element->content_ofs_x,
                               used_element->content_ofs_y);
  }

  // Paint children.
  OnPaintChildren(paint_props);

#ifdef TB_RUNTIME_DEBUG_INFO
  if (TB_DEBUG_SETTING(util::DebugInfo::Setting::kLayoutSizing)) {
    // Layout debug painting. Paint recently layouted elements with red and
    // recently measured elements with yellow.
    // Invalidate to keep repainting until we've timed out (so it's removed).
    const uint64_t debug_time = 300;
    const uint64_t now = util::GetTimeMS();
    if (now < last_layout_time + debug_time) {
      Renderer::get()->DrawRect(local_rect, Color(255, 30, 30, 200));
      Invalidate();
    }
    if (now < last_measure_time + debug_time) {
      Renderer::get()->DrawRect(local_rect.Shrink(1, 1),
                                Color(255, 255, 30, 200));
      Invalidate();
    }
  }
#endif  // TB_RUNTIME_DEBUG_INFO

  if (used_element) {
    Renderer::get()->Translate(-used_element->content_ofs_x,
                               -used_element->content_ofs_y);
  }

  Renderer::get()->Translate(-trns_x, -trns_y);
  Renderer::get()->SetOpacity(old_opacity);
}

bool Element::InvokeEvent(ElementEvent& ev) {
  ev.target = this;

  // First call the global listener about this event.
  // Who knows, maybe some listener will block the event or cause us
  // to be deleted.
  WeakElementPointer this_element(this);
  if (ElementListener::InvokeElementInvokeEvent(this, ev)) return true;

  if (!this_element.Get()) {
    return true;  // We got removed so we actually handled this event.
  }

  if (ev.type == EventType::kChanged) {
    InvalidateSkinStates();
    m_connection.SyncFromElement(this);
  }

  if (!this_element.Get()) {
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

  // Call OnEvent on this elements and travel up through its parents if not
  // handled.
  bool handled = false;
  Element* tmp = this;
  while (tmp && !(handled = tmp->OnEvent(ev))) {
    tmp = tmp->GetEventDestination();
  }
  return handled;
}

void Element::StartLongClickTimer(bool touch) {
  StopLongClickTimer();
  m_long_click_timer = new LongClickTimer(this, touch);
}

void Element::StopLongClickTimer() {
  if (!m_long_click_timer) return;
  delete m_long_click_timer;
  m_long_click_timer = nullptr;
}

bool Element::InvokePointerDown(int x, int y, int click_count,
                                ModifierKeys modifierkeys, bool touch) {
  // If we have a captured element then the pointer event was handled since
  // focus is changed here.
  if (!captured_element) {
    SetCapturedElement(GetElementAt(x, y, true));
    SetHoveredElement(captured_element, touch);
    // captured_button = button;

    // Hide focus when we use the pointer, if it's not on the focused element.
    if (focused_element != captured_element) SetAutoFocusState(false);

    // Start long click timer. Only for touch events for now.
    if (touch && captured_element && captured_element->GetWantLongClick()) {
      captured_element->StartLongClickTimer(touch);
    }

    // Get the closest parent window and bring it to the top.
    Window* window =
        captured_element ? captured_element->GetParentWindow() : nullptr;
    if (window) {
      window->Activate();
    }
  }
  if (captured_element) {
    // Check if there's any started scroller that should be stopped.
    Element* tmp = captured_element;
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

    // Focus the captured element or the closest focusable parent if it isn't
    // focusable.
    Element* focus_target = captured_element;
    while (focus_target) {
      if (focus_target->SetFocus(FocusReason::kPointer)) {
        break;
      }
      focus_target = focus_target->m_parent;
    }
  }
  if (captured_element) {
    captured_element->ConvertFromRoot(x, y);
    pointer_move_element_x = pointer_down_element_x = x;
    pointer_move_element_y = pointer_down_element_y = y;
    ElementEvent ev(EventType::kPointerDown, x, y, touch, modifierkeys);
    ev.count = click_count;
    captured_element->InvokeEvent(ev);
    return true;
  }

  return false;
}

bool Element::InvokePointerUp(int x, int y, ModifierKeys modifierkeys,
                              bool touch) {
  // If we have a captured element then we have a focused element so the pointer
  // up event was handled
  if (captured_element) {
    captured_element->ConvertFromRoot(x, y);
    ElementEvent ev_up(EventType::kPointerUp, x, y, touch, modifierkeys);
    ElementEvent ev_click(EventType::kClick, x, y, touch, modifierkeys);
    captured_element->InvokeEvent(ev_up);
    if (!cancel_click && captured_element &&
        captured_element->GetHitStatus(x, y) != HitStatus::kNoHit) {
      captured_element->InvokeEvent(ev_click);
    }
    if (captured_element) {  // && button == captured_button
      captured_element->ReleaseCapture();
    }
    return true;
  }

  return false;
}

void Element::MaybeInvokeLongClickOrContextMenu(bool touch) {
  StopLongClickTimer();
  if (captured_element == this && !cancel_click &&
      captured_element->GetHitStatus(pointer_move_element_x,
                                     pointer_move_element_y) !=
          HitStatus::kNoHit) {
    // Invoke long click.
    ElementEvent ev_long_click(EventType::kLongClick, pointer_move_element_x,
                               pointer_move_element_y, touch,
                               ModifierKeys::kNone);
    bool handled = captured_element->InvokeEvent(ev_long_click);
    if (!handled) {
      // Long click not handled so invoke a context menu event instead.
      ElementEvent ev_context_menu(
          EventType::kContextMenu, pointer_move_element_x,
          pointer_move_element_y, touch, ModifierKeys::kNone);
      handled = captured_element->InvokeEvent(ev_context_menu);
    }
    // If any event was handled, suppress click when releasing pointer.
    if (handled) {
      cancel_click = true;
    }
  }
}

void Element::InvokePointerMove(int x, int y, ModifierKeys modifierkeys,
                                bool touch) {
  SetHoveredElement(GetElementAt(x, y, true), touch);
  Element* target = captured_element ? captured_element : hovered_element;

  if (target) {
    target->ConvertFromRoot(x, y);
    pointer_move_element_x = x;
    pointer_move_element_y = y;

    ElementEvent ev(EventType::kPointerMove, x, y, touch, modifierkeys);
    if (target->InvokeEvent(ev)) {
      return;
    }
    // The move event was not handled, so handle panning of scrollable elements.
    HandlePanningOnMove(x, y);
  }
}

void Element::HandlePanningOnMove(int x, int y) {
  if (!captured_element) return;

  // Check pointer movement.
  const int dx = pointer_down_element_x - x;
  const int dy = pointer_down_element_y - y;
  const int threshold = util::GetPanThreshold();
  const bool maybe_start_panning_x = std::abs(dx) >= threshold;
  const bool maybe_start_panning_y = std::abs(dy) >= threshold;

  // Do panning, or attempt starting panning (we don't know if any element is
  // scrollable yet).
  if (captured_element->m_packed.is_panning || maybe_start_panning_x ||
      maybe_start_panning_y) {
    // The threshold is met for not invoking any long click.
    captured_element->StopLongClickTimer();

    int start_compensation_x = 0, start_compensation_y = 0;
    if (!captured_element->m_packed.is_panning) {
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
    Scroller* scroller = captured_element->GetReadyScroller(dx != 0, dy != 0);
    if (!scroller) {
      return;
    }

    int old_translation_x = 0, old_translation_y = 0;
    captured_element->GetScrollRoot()->GetChildTranslation(old_translation_x,
                                                           old_translation_y);

    if (scroller->OnPan(dx + start_compensation_x, dy + start_compensation_y)) {
      // Scroll delta changed, so we are now panning!
      captured_element->m_packed.is_panning = true;
      cancel_click = true;

      // If the captured element (or its scroll root) has panned, we have to
      // compensate the pointer down coordinates so we won't accumulate the
      // difference the following pan.
      int new_translation_x = 0, new_translation_y = 0;
      captured_element->GetScrollRoot()->GetChildTranslation(new_translation_x,
                                                             new_translation_y);
      pointer_down_element_x +=
          new_translation_x - old_translation_x + start_compensation_x;
      pointer_down_element_y +=
          new_translation_y - old_translation_y + start_compensation_y;
    }
  }
}

bool Element::InvokeWheel(int x, int y, int delta_x, int delta_y,
                          ModifierKeys modifierkeys) {
  SetHoveredElement(GetElementAt(x, y, true), true);

  // If we have a target then the wheel event should be consumed.
  Element* target = captured_element ? captured_element : hovered_element;
  if (!target) {
    return false;
  }
  target->ConvertFromRoot(x, y);
  pointer_move_element_x = x;
  pointer_move_element_y = y;
  ElementEvent ev(EventType::kWheel, x, y, true, modifierkeys);
  ev.delta_x = delta_x;
  ev.delta_y = delta_y;
  target->InvokeEvent(ev);
  return true;
}

bool Element::InvokeKey(int key, SpecialKey special_key,
                        ModifierKeys modifierkeys, bool down) {
  bool handled = false;
  if (focused_element) {
    // Emulate a click on the focused element when pressing space or enter.
    if (!any(modifierkeys) && focused_element->GetClickByKey() &&
        !focused_element->GetDisabled() && !focused_element->GetIsDying() &&
        (special_key == SpecialKey::kEnter || key == ' ')) {
      // Set the pressed state while the key is down, if it didn't already have
      // the pressed state.
      static bool check_pressed_state = true;
      static bool had_pressed_state = false;
      if (down && check_pressed_state) {
        had_pressed_state = focused_element->GetState(Element::State::kPressed);
        check_pressed_state = false;
      }
      if (!down) {
        check_pressed_state = true;
      }

      if (!had_pressed_state) {
        focused_element->SetState(Element::State::kPressed, down);
        focused_element->m_packed.has_key_pressed_state = down;
      }

      // Invoke the click event.
      if (!down) {
        ElementEvent ev(EventType::kClick, m_rect.w / 2, m_rect.h / 2, true);
        focused_element->InvokeEvent(ev);
      }
      handled = true;
    } else {
      // Invoke the key event on the focused element.
      ElementEvent ev(down ? EventType::kKeyDown : EventType::kKeyUp);
      ev.key = key;
      ev.special_key = special_key;
      ev.modifierkeys = modifierkeys;
      handled = focused_element->InvokeEvent(ev);
    }
  }

  // Move focus between elements.
  if (down && !handled && special_key == SpecialKey::kTab) {
    handled = MoveFocus(!any(modifierkeys & ModifierKeys::kShift));

    // Show the focus when we move it by keyboard.
    if (handled) {
      SetAutoFocusState(true);
    }
  }
  return handled;
}

void Element::ReleaseCapture() {
  if (this == captured_element) {
    SetCapturedElement(nullptr);
  }
}

void Element::ConvertToRoot(int& x, int& y) const {
  const Element* tmp = this;
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

void Element::ConvertFromRoot(int& x, int& y) const {
  const Element* tmp = this;
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
void Element::SetHoveredElement(Element* element, bool touch) {
  if (Element::hovered_element == element) {
    return;
  }
  if (element && element->GetState(Element::State::kDisabled)) {
    return;
  }

  // We may apply hover state automatically so the element might need to be
  // updated.
  if (Element::hovered_element) {
    Element::hovered_element->Invalidate();
    Element::hovered_element->InvalidateSkinStates();
  }

  Element::hovered_element = element;

  if (Element::hovered_element) {
    Element::hovered_element->Invalidate();
    Element::hovered_element->InvalidateSkinStates();

    // Cursor based movement should set hover state automatically, but touch
    // events should not (since touch doesn't really move unless pressed).
    Element::hovered_element->m_packed.no_automatic_hover_state = touch;
  }
}

// static
void Element::SetCapturedElement(Element* element) {
  if (Element::captured_element == element) {
    return;
  }
  if (element && element->GetState(Element::State::kDisabled)) {
    return;
  }

  if (Element::captured_element) {
    // Stop panning when capture change (most likely changing to nullptr because
    // of InvokePointerUp).
    // Notify any active scroller so it may begin scrolling.
    if (Scroller* scroller = Element::captured_element->FindStartedScroller()) {
      if (Element::captured_element->m_packed.is_panning) {
        scroller->OnPanReleased();
      } else {
        scroller->Stop();
      }
    }
    Element::captured_element->m_packed.is_panning = false;

    // We apply pressed state automatically so the element might need to be
    // updated.
    Element::captured_element->Invalidate();
    Element::captured_element->InvalidateSkinStates();

    Element::captured_element->StopLongClickTimer();
  }
  cancel_click = false;

  Element* old_capture = Element::captured_element;

  Element::captured_element = element;

  if (old_capture) old_capture->OnCaptureChanged(false);

  if (Element::captured_element) {
    Element::captured_element->Invalidate();
    Element::captured_element->InvalidateSkinStates();
    Element::captured_element->OnCaptureChanged(true);
  }
}

bool Element::SetFontDescription(const FontDescription& font_desc) {
  if (m_font_desc == font_desc) return true;

  // Set the font description only if we have a matching font, or succeed
  // creating one.
  if (resources::FontManager::get()->HasFontFace(font_desc)) {
    m_font_desc = font_desc;
  } else if (resources::FontManager::get()->CreateFontFace(font_desc)) {
    m_font_desc = font_desc;
  } else {
    return false;
  }

  InvokeFontChanged();
  return true;
}

void Element::InvokeFontChanged() {
  OnFontChanged();

  // Recurse to children that inherit the font.
  for (Element* child = GetFirstChild(); child; child = child->GetNext()) {
    if (child->m_font_desc.GetFontFaceID() == 0) {
      child->InvokeFontChanged();
    }
  }
}

FontDescription Element::GetCalculatedFontDescription() const {
  const Element* tmp = this;
  while (tmp) {
    if (tmp->m_font_desc.GetFontFaceID() != 0) {
      return tmp->m_font_desc;
    }
    tmp = tmp->m_parent;
  }
  return resources::FontManager::get()->GetDefaultFontDescription();
}

resources::FontFace* Element::GetFont() const {
  return resources::FontManager::get()->GetFontFace(
      GetCalculatedFontDescription());
}

}  // namespace tb
