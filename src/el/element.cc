/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdarg>

#include "el/element.h"
#include "el/element_listener.h"
#include "el/elements/form.h"
#include "el/elements/parts/scroller.h"
#include "el/elements/tab_container.h"
#include "el/elements/text_box.h"
#include "el/event_handler.h"
#include "el/graphics/renderer.h"
#include "el/list_item.h"
#include "el/parsing/element_inflater.h"
#include "el/parsing/parse_node.h"
#include "el/text/font_manager.h"
#include "el/util/debug.h"
#include "el/util/math.h"
#include "el/util/metrics.h"
#include "el/util/string.h"
#include "el/value.h"

namespace el {

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
    assert(msg->message_id() == TBIDC("LongClickTimer"));
    m_element->MaybeInvokeLongClickOrContextMenu(m_touch);
  }

 private:
  Element* m_element;
  bool m_touch;
};

Element::PaintProps::PaintProps() {
  // Set the default properties, used for the root elements
  // calling InvokePaint. The base values for all inheritance.
  text_color = Skin::get()->default_text_color();
}

void Element::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Element, Value::Type::kNull, ElementZ::kTop);
}

Element::Element() = default;

Element::~Element() {
  // A element must be removed from parent before deleted.
  RemoveFromParent();
  assert(!m_parent);
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

  StopLongClickTimer();

  assert(!m_listeners
              .HasLinks());  // There's still listeners added to this element!
}

bool Element::LoadFile(const char* filename) {
  return parsing::ElementFactory::get()->LoadFile(this, filename);
}

bool Element::LoadData(const char* data_str, size_t data_length) {
  return parsing::ElementFactory::get()->LoadData(this, data_str, data_length);
}

void Element::LoadNodeTree(parsing::ParseNode* node) {
  return parsing::ElementFactory::get()->LoadNodeTree(this, node);
}

void Element::LoadNodeTree(const dsl::Node& node) {
  parsing::ParseNode parse_node;
  parse_node.Add(node.parse_node());
  LoadNodeTree(&parse_node);
}

// Sets the id from the given node.
void Element::SetIdFromNode(TBID* id, parsing::ParseNode* node) {
  if (!node) return;
  if (node->value().is_string()) {
    id->reset(node->value().as_string());
  } else {
    id->reset(node->value().as_integer());
  }
}

void Element::OnInflate(const parsing::InflateInfo& info) {
  Element::SetIdFromNode(&id(), info.node->GetNode("id"));
  Element::SetIdFromNode(&group_id(), info.node->GetNode("group-id"));

  if (info.sync_type == Value::Type::kFloat) {
    set_double_value(info.node->GetValueFloat("value", 0));
  } else {
    set_value(info.node->GetValueInt("value", 0));
  }

  if (auto data_node = info.node->GetNode("data")) {
    data.Copy(data_node->value());
  }

  set_group_root(
      info.node->GetValueInt("is-group-root", is_group_root()) ? true : false);

  set_focusable(info.node->GetValueInt("is-focusable", is_focusable()) ? true
                                                                       : false);

  set_long_clickable(
      info.node->GetValueInt("want-long-click", is_long_clickable()) ? true
                                                                     : false);

  set_ignoring_input(info.node->GetValueInt("ignore-input", is_ignoring_input())
                         ? true
                         : false);

  set_opacity(info.node->GetValueFloat("opacity", opacity()));

  if (const char* text = info.node->GetValueString("text", nullptr)) {
    set_text(text);
  }

  if (const char* connection =
          info.node->GetValueStringRaw("connection", nullptr)) {
    // If we already have a element value with this name, just connect to it and
    // the element will adjust its value to it. Otherwise create a new element
    // value, and give it the value we got from the resource.
    if (ElementValue* value = ElementValueGroup::get()->GetValue(connection)) {
      Connect(value);
    } else if (ElementValue* value =
                   ElementValueGroup::get()->CreateValueIfNeeded(
                       connection, info.sync_type)) {
      value->SetFromElement(this);
      Connect(value);
    }
  }
  if (const char* axis = info.node->GetValueString("axis", nullptr)) {
    set_axis(el::from_string(axis, Axis::kY));
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
    set_gravity(g);
  }
  if (const char* visibility_str =
          info.node->GetValueString("visibility", nullptr)) {
    set_visibility(from_string(visibility_str, visibility()));
  }
  if (const char* state = info.node->GetValueString("state", nullptr)) {
    if (strstr(state, "disabled")) {
      set_state(Element::State::kDisabled, true);
    }
  }
  if (const char* skin = info.node->GetValueString("skin", nullptr)) {
    set_background_skin(skin);
  }
  if (auto lp = info.node->GetNode("lp")) {
    LayoutParams layout_params;
    if (this->layout_params()) {
      layout_params = *this->layout_params();
    }
    auto dc = Skin::get()->dimension_converter();
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
    set_layout_params(layout_params);
  }

  set_tooltip(info.node->GetValueString("tooltip", nullptr));

  // Add the new element to the hiearchy if not already added.
  if (!parent()) info.target->AddChild(this, info.target->z_inflate());

  // Read the font now when the element is in the hiearchy so inheritance works.
  if (auto font = info.node->GetNode("font")) {
    FontDescription fd = computed_font_description();
    if (const char* size = font->GetValueString("size", nullptr)) {
      int new_size =
          Skin::get()->dimension_converter()->GetPxFromString(size, fd.size());
      fd.set_size(new_size);
    }
    if (const char* name = font->GetValueString("name", nullptr)) {
      fd.set_id(name);
    }
    set_font_description(fd);
  }

  info.target->OnInflateChild(this);

  if (auto rect_node = info.node->GetNode("rect")) {
    auto dc = Skin::get()->dimension_converter();
    Value& val = rect_node->value();
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
  if (!computed_visibility() && !m_rect.empty()) {
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

Element* Element::GetElementByIdInternal(const TBID& id,
                                         const util::tb_type_id_t type_id) {
  if (m_id == id && (!type_id || IsOfTypeId(type_id))) {
    return this;
  }
  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (Element* sub_child = child->GetElementByIdInternal(id, type_id)) {
      return sub_child;
    }
  }
  return nullptr;
}

bool Element::GetElementsById(std::vector<LookupPair> els) {
  bool any_failed = false;
  for (auto& pair : els) {
    auto el = GetElementByIdInternal(pair.id, pair.type_id);
    if (!el) {
      any_failed = true;
    }
    *pair.out_ptr = el;
  }
  return !any_failed;
}

std::string Element::GetTextById(const TBID& id) {
  if (auto element = GetElementById<Element>(id)) {
    return element->text();
  }
  return "";
}

int Element::GetValueById(const TBID& id) {
  if (auto element = GetElementById<Element>(id)) {
    return element->value();
  }
  return 0;
}

void Element::set_id(const TBID& id) {
  m_id = id;
  InvalidateSkinStates();
}

void Element::set_state_raw(Element::State state) {
  if (m_state == state) return;
  m_state = state;
  Invalidate();
  InvalidateSkinStates();
}

void Element::set_state(Element::State state, bool on) {
  set_state_raw(on ? m_state | state : m_state & ~state);
}

Element::State Element::computed_state() const {
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
  } else if (this == focused_element && IsOfType<elements::TextBox>()) {
#ifdef EL_ALWAYS_SHOW_EDIT_FOCUS
    state |= Element::State::kFocused;
#endif  // EL_ALWAYS_SHOW_EDIT_FOCUS
  }
  return state;
}

// static
void Element::set_auto_focus_state(bool on) {
  if (show_focus_state == on) {
    return;
  }
  show_focus_state = on;
  if (focused_element) {
    focused_element->Invalidate();
  }
}

void Element::set_opacity(float opacity) {
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

void Element::set_visibility(Visibility vis) {
  if (m_packed.visibility == static_cast<int>(vis)) {
    return;
  }

  // Invalidate after making it invisible will do nothing.
  if (vis != Visibility::kVisible) {
    Invalidate();
  }
  if (vis == Visibility::kGone) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  Visibility old_vis = visibility();
  m_packed.visibility = static_cast<int>(vis);

  Invalidate();
  if (old_vis == Visibility::kGone) {
    InvalidateLayout(InvalidationMode::kRecursive);
  }

  OnVisibilityChanged();
}

Visibility Element::visibility() const {
  return static_cast<Visibility>(m_packed.visibility);
}

bool Element::computed_visibility() const {
  const Element* tmp = this;
  while (tmp) {
    if (tmp->opacity() == 0 || tmp->visibility() != Visibility::kVisible) {
      return false;
    }
    tmp = tmp->m_parent;
  }
  return true;
}

bool Element::is_enabled() const {
  const Element* tmp = this;
  while (tmp) {
    if (tmp->has_state(Element::State::kDisabled)) {
      return false;
    }
    tmp = tmp->m_parent;
  }
  return true;
}

void Element::RemoveFromParent() {
  if (parent()) {
    parent()->RemoveChild(this);
  }
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
  assert(child->m_parent == this);

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

void Element::DeleteChild(Element* child, InvokeInfo info) {
  RemoveChild(child);
  delete child;
}

void Element::ReplaceChild(Element* old_child, Element* new_child,
                           InvokeInfo info) {
  AddChildRelative(new_child, ElementZRel::kAfter, old_child, info);
  DeleteChild(old_child, info);
}

void Element::DeleteAllChildren() {
  while (Element* child = first_child()) {
    DeleteChild(child);
  }
}

void Element::set_z(ElementZ z) {
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

void Element::set_gravity(Gravity g) {
  if (m_gravity == g) return;
  m_gravity = g;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Element::set_background_skin(const TBID& skin_bg, InvokeInfo info) {
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

SkinElement* Element::background_skin_element() {
  ElementSkinConditionContext context(this);
  Element::State state = computed_state();
  return Skin::get()->GetSkinElementStrongOverride(
      m_skin_bg, static_cast<Element::State>(state), context);
}

Element* Element::FindScrollableElement(bool scroll_x, bool scroll_y) {
  Element* candidate = this;
  while (candidate) {
    ScrollInfo scroll_info = candidate->scroll_info();
    if ((scroll_x && scroll_info.CanScrollX()) ||
        (scroll_y && scroll_info.CanScrollY())) {
      return candidate;
    }
    candidate = candidate->parent();
  }
  return nullptr;
}

elements::parts::Scroller* Element::FindStartedScroller() {
  Element* candidate = this;
  while (candidate) {
    if (candidate->m_scroller && candidate->m_scroller->is_started()) {
      return candidate->m_scroller.get();
    }
    candidate = candidate->parent();
  }
  return nullptr;
}

elements::parts::Scroller* Element::GetReadyScroller(bool scroll_x,
                                                     bool scroll_y) {
  if (auto scroller = FindStartedScroller()) return scroller;
  // We didn't have any active scroller, so create one for the nearest
  // scrollable parent.
  if (Element* scrollable_element = FindScrollableElement(scroll_x, scroll_y)) {
    return scrollable_element->scroller();
  }
  return nullptr;
}

elements::parts::Scroller* Element::scroller() {
  if (!m_scroller) {
    m_scroller = std::make_unique<elements::parts::Scroller>(this);
  }
  return m_scroller.get();
}

void Element::ScrollToSmooth(int x, int y) {
  ScrollInfo info = scroll_info();
  int dx = x - info.x;
  int dy = y - info.y;
  if (auto scroller = GetReadyScroller(dx != 0, dy != 0)) {
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

  if (auto scroller = GetReadyScroller(dx != 0, dy != 0)) {
    scroller->OnScrollBy(dx, dy, true);
  }
}

void Element::ScrollBy(int dx, int dy) {
  ScrollInfo info = scroll_info();
  ScrollTo(info.x + dx, info.y + dy);
}

void Element::ScrollByRecursive(int* dx, int* dy) {
  Element* tmp = this;
  while (tmp) {
    ScrollInfo old_info = tmp->scroll_info();
    tmp->ScrollTo(old_info.x + *dx, old_info.y + *dy);
    ScrollInfo new_info = tmp->scroll_info();
    *dx -= new_info.x - old_info.x;
    *dy -= new_info.y - old_info.y;
    if (!*dx && !*dy) {
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
  const ScrollInfo info = scroll_info();
  int new_x = info.x;
  int new_y = info.y;

  const Rect visible_rect = padding_rect().Offset(info.x, info.y);

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

bool Element::set_focus(FocusReason reason, InvokeInfo info) {
  if (focused_element == this) return true;
  if (!is_enabled() || !is_focusable() || !computed_visibility() ||
      is_dying()) {
    return false;
  }

  // Update forms last focus.
  auto form = parent_form();
  if (form) {
    form->set_last_focus(this);
    // If not active, just return. We should get focus when the form is
    // activated.
    // Exception for forms that doesn't activate. They may contain focusable
    // elements.
    if (!form->is_active() &&
        any(form->settings() & elements::FormSettings::kCanActivate)) {
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
    if (Element* old = old_focus.get()) {
      // The currently focused element still has the pressed state set by the
      // emulated click (by keyboard), so unset it before we unfocus it so it's
      // not stuck in pressed state.
      if (old->m_packed.has_key_pressed_state) {
        old->set_state(Element::State::kPressed, false);
        old->m_packed.has_key_pressed_state = false;
      }
      old->OnFocusChanged(false);
    }
    if (old_focus.get()) {
      ElementListener::InvokeElementFocusChanged(old_focus.get(), false);
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

bool Element::set_focus_recursive(FocusReason reason) {
  // Search for a child element that accepts focus.
  Element* child = first_child();
  while (child) {
    if (child->set_focus(FocusReason::kUnknown)) {
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

  Element* root = origin->parent_form();
  if (!root) {
    root = origin->parent_root();
  }

  Element* current = origin;
  while (current) {
    current = forward ? current->GetNextDeep() : current->GetPrevDeep();
    // Wrap around if we reach the end/beginning.
    if (!current || !root->IsAncestorOf(current)) {
      current = forward ? root->first_child() : root->GetLastLeaf();
    }
    // Break if we reached the origin again (we're not finding anything else).
    if (current == origin) {
      break;
    }
    // Try to focus what we found.
    if (current && current->set_focus(FocusReason::kNavigation)) {
      return true;
    }
  }
  return false;
}

Element* Element::GetNextDeep(const Element* bounding_ancestor) const {
  if (m_children.GetFirst()) {
    return first_child();
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
    element = element->last_child();
  }
  return element;
}

Element* Element::GetLastLeaf() const {
  if (Element* element = last_child()) {
    while (element->last_child()) {
      element = element->last_child();
    }
    return element;
  }
  return nullptr;
}

bool Element::is_interactable() const {
  return !(m_opacity == 0 || is_ignoring_input() ||
           has_state(Element::State::kDisabled) || is_dying() ||
           visibility() != Visibility::kVisible);
}

HitStatus Element::GetHitStatus(int x, int y) {
  if (!is_interactable()) return HitStatus::kNoHit;
  return x >= 0 && y >= 0 && x < m_rect.w && y < m_rect.h ? HitStatus::kHit
                                                          : HitStatus::kNoHit;
}

Element* Element::GetElementAt(int x, int y, bool include_children) const {
  int child_translation_x;
  int child_translation_y;
  GetChildTranslation(&child_translation_x, &child_translation_y);
  x -= child_translation_x;
  y -= child_translation_y;

  Element* tmp = first_child();
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
  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (i++ == index) return child;
  }
  return nullptr;
}

int Element::GetIndexFromChild(Element* child) const {
  assert(child->parent() == this);
  int i = 0;
  for (Element *tmp = first_child(); tmp; tmp = tmp->GetNext(), ++i) {
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
    other_element = other_element->event_destination();
  }
  return false;
}

Element* Element::parent_root() {
  Element* tmp = this;
  while (tmp->m_parent) {
    tmp = tmp->m_parent;
  }
  return tmp;
}

elements::Form* Element::parent_form() {
  Element* tmp = this;
  while (tmp && !tmp->IsOfType<elements::Form>()) {
    tmp = tmp->m_parent;
  }
  return static_cast<elements::Form*>(tmp);
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

void Element::AddEventHandler(EventHandler* event_handler) {
  event_handlers_.AddLast(event_handler);
}

void Element::RemoveEventHandler(EventHandler* event_handler) {
  event_handlers_.Remove(event_handler);
}

bool Element::OnEvent(const Event& ev) {
  auto it = event_handlers_.IterateForward();
  while (auto event_handler = it.GetAndStep()) {
    if (event_handler->OnEvent(ev)) {
      return true;
    }
  }
  return false;
}

void Element::OnPaintChildren(const PaintProps& paint_props) {
  if (!m_children.GetFirst()) return;

  // Translate renderer with child translation.
  int child_translation_x;
  int child_translation_y;
  GetChildTranslation(&child_translation_x, &child_translation_y);
  Renderer::get()->Translate(child_translation_x, child_translation_y);

  Rect clip_rect = Renderer::get()->clip_rect();

  // Invoke paint on all children that are in the current visible rect.
  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (clip_rect.intersects(child->m_rect)) {
      child->InvokePaint(paint_props);
    }
  }

  // Invoke paint of overlay elements on all children that are in the current
  // visible rect.
  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (clip_rect.intersects(child->m_rect) &&
        child->visibility() == Visibility::kVisible) {
      auto skin_element = child->background_skin_element();
      if (skin_element && skin_element->has_overlay_elements()) {
        // Update the renderer with the elements opacity.
        Element::State state = child->computed_state();
        float old_opacity = Renderer::get()->opacity();
        float opacity =
            old_opacity * child->CalculateOpacityInternal(state, skin_element);
        if (opacity > 0) {
          Renderer::get()->set_opacity(opacity);

          ElementSkinConditionContext context(child);
          Skin::get()->PaintSkinOverlay(child->m_rect, skin_element,
                                        static_cast<Element::State>(state),
                                        context);

          Renderer::get()->set_opacity(old_opacity);
        }
      }
    }
  }

  // Draw generic focus skin if the focused element is one of the children, and
  // the skin doesn't have a skin state for focus which would already be
  // painted.
  if (focused_element && focused_element->m_parent == this) {
    ElementSkinConditionContext context(focused_element);
    auto skin_element = focused_element->background_skin_element();
    if (!skin_element ||
        !skin_element->has_state(Element::State::kFocused, context)) {
      Element::State state = focused_element->computed_state();
      if (any(state & Element::State::kFocused)) {
        Skin::get()->PaintSkin(focused_element->m_rect, TBIDC("generic_focus"),
                               static_cast<Element::State>(state), context);
      }
    }
  }

  Renderer::get()->Translate(-child_translation_x, -child_translation_y);
}

void Element::OnResized(int old_w, int old_h) {
  int dw = m_rect.w - old_w;
  int dh = m_rect.h - old_h;
  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (child->visibility() == Visibility::kGone) continue;
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
  if (child->visibility() == Visibility::kGone) return;

  // If the child pull towards only one edge (per axis), stick to that edge
  // and use the preferred size. Otherwise fill up all available space.
  Rect padding_rect = this->padding_rect();
  Rect child_rect = padding_rect;
  Gravity gravity = child->gravity();
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

void Element::set_text_format(const char* format, ...) {
  va_list va;
  va_start(va, format);
  set_text(el::util::format_string(format, va));
  va_end(va);
}

Rect Element::padding_rect() {
  Rect padding_rect(0, 0, m_rect.w, m_rect.h);
  if (auto e = background_skin_element()) {
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

  auto bg_skin = background_skin_element();
  int horizontal_padding =
      bg_skin ? bg_skin->padding_left + bg_skin->padding_right : 0;
  int vertical_padding =
      bg_skin ? bg_skin->padding_top + bg_skin->padding_bottom : 0;
  SizeConstraints inner_sc =
      constraints.ConstrainByPadding(horizontal_padding, vertical_padding);

  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (child->visibility() == Visibility::kGone) {
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

  auto e = background_skin_element();
  if (!e) {
    return ps;
  }

  // Override the elements preferences with skin attributes that has been
  // specified.
  // If not set by the element, calculate based on the intrinsic size of the
  // skin.
  const int skin_intrinsic_w = e->intrinsic_width();
  if (e->preferred_width() != kSkinValueNotSpecified) {
    ps.pref_w = e->preferred_width();
  } else if (ps.pref_w == 0 && skin_intrinsic_w != kSkinValueNotSpecified) {
    ps.pref_w = skin_intrinsic_w;
  } else {
    // Grow by padding to get the preferred size from preferred content size.
    ps.min_w += e->padding_left + e->padding_right;
    ps.pref_w += e->padding_left + e->padding_right;
  }

  const int skin_intrinsic_h = e->intrinsic_height();
  if (e->preferred_height() != kSkinValueNotSpecified) {
    ps.pref_h = e->preferred_height();
  } else if (ps.pref_h == 0 && skin_intrinsic_h != kSkinValueNotSpecified) {
    ps.pref_h = skin_intrinsic_h;
  } else {
    // Grow by padding to get the preferred size from preferred content size.
    ps.min_h += e->padding_top + e->padding_bottom;
    ps.pref_h += e->padding_top + e->padding_bottom;
  }

  if (e->min_width() != kSkinValueNotSpecified) {
    ps.min_w = e->min_width();
  } else {
    ps.min_w = std::max(ps.min_w, e->intrinsic_min_width());
  }

  if (e->min_height() != kSkinValueNotSpecified) {
    ps.min_h = e->min_height();
  } else {
    ps.min_h = std::max(ps.min_h, e->intrinsic_min_height());
  }

  if (e->max_width() != kSkinValueNotSpecified) {
    ps.max_w = e->max_width();
  } else {
    ps.max_w += e->padding_left + e->padding_right;
  }

  if (e->max_height() != kSkinValueNotSpecified) {
    ps.max_h = e->max_height();
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
        m_cached_ps.size_dependency == SizeDependency::kNone) {
      /*||
                        // FIX: These optimizations would probably be good.
         Keeping
                        //      disabled for now because it needs testing.
                        // If *only* width depend on height, only the height
         matter
                        (m_cached_ps.size_dependency ==
         SizeDependency::kWidthOnHeight &&
                        m_cached_sc.available_h == constraints.available_h) ||
                        // If *only* height depend on width, only the width
         matter
                        (m_cached_ps.size_dependency ==
         SizeDependency::kHeightOnWidth &&
                        m_cached_sc.available_w == constraints.available_w)*/
      return m_cached_ps;
    }
  }

  // Measure and save to cache.
  EL_IF_DEBUG_SETTING(util::DebugInfo::Setting::kLayoutSizing,
                      last_measure_time = util::GetTimeMS());
  m_packed.is_cached_ps_valid = 1;
  m_cached_ps = OnCalculatePreferredSize(constraints);
  m_cached_sc = constraints;

  // Override the calculated ps with any specified layout parameter.
  if (m_layout_params) {
#define LP_OVERRIDE(param)                                    \
  if (m_layout_params->param != LayoutParams::kUnspecified) { \
    m_cached_ps.param = m_layout_params->param;               \
  }
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

void Element::set_layout_params(const LayoutParams& lp) {
  if (!m_layout_params) {
    m_layout_params = std::make_unique<LayoutParams>();
  }
  *m_layout_params = lp;
  m_packed.is_cached_ps_valid = 0;
  InvalidateLayout(InvalidationMode::kRecursive);
}

void Element::InvalidateLayout(InvalidationMode il) {
  m_packed.is_cached_ps_valid = 0;
  if (visibility() == Visibility::kGone) {
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
  if (auto skin_elm = background_skin_element()) {
    if (skin_elm->id != m_skin_bg_expected) {
      OnSkinChanged();
      m_skin_bg_expected = skin_elm->id;
      InvalidateLayout(InvalidationMode::kRecursive);
    }
  }

  for (Element* child = first_child(); child; child = child->GetNext()) {
    child->InvokeSkinUpdatesInternal(true);
  }
}

void Element::InvokeProcessInternal() {
  OnProcess();

  for (Element* child = first_child(); child; child = child->GetNext()) {
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

  for (Element* child = first_child(); child; child = child->GetNext())
    child->InvokeProcessStates(true);
}

float Element::CalculateOpacityInternal(Element::State state,
                                        SkinElement* skin_element) const {
  float opacity = m_opacity;
  if (skin_element) {
    opacity *= skin_element->opacity;
  }
  if (any(state & Element::State::kDisabled)) {
    opacity *= Skin::get()->default_disabled_opacity();
  }
  return opacity;
}

void Element::InvokePaint(const PaintProps& parent_paint_props) {
  // Don't paint invisible elements
  if (m_opacity == 0 || m_rect.empty() ||
      visibility() != Visibility::kVisible) {
    return;
  }

  Element::State state = computed_state();
  auto skin_element = background_skin_element();

  // Multiply current opacity with element opacity, skin opacity and state
  // opacity.
  float old_opacity = Renderer::get()->opacity();
  float opacity = old_opacity * CalculateOpacityInternal(state, skin_element);
  if (opacity == 0) return;

  // FIX: This does not give the correct result! Must use a new render target!
  Renderer::get()->set_opacity(opacity);

  int trns_x = m_rect.x, trns_y = m_rect.y;
  Renderer::get()->Translate(trns_x, trns_y);

  // Paint background skin.
  Rect local_rect(0, 0, m_rect.w, m_rect.h);
  ElementSkinConditionContext context(this);
  auto used_element = Skin::get()->PaintSkin(
      local_rect, skin_element, static_cast<Element::State>(state), context);
  assert(!!used_element == !!skin_element);

  EL_IF_DEBUG_SETTING(
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

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kLayoutSizing)) {
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
#endif  // EL_RUNTIME_DEBUG_INFO

  if (used_element) {
    Renderer::get()->Translate(-used_element->content_ofs_x,
                               -used_element->content_ofs_y);
  }

  Renderer::get()->Translate(-trns_x, -trns_y);
  Renderer::get()->set_opacity(old_opacity);
}

bool Element::InvokeEvent(Event ev) {
  ev.target = this;

  // First call the global listener about this event.
  // Who knows, maybe some listener will block the event or cause us
  // to be deleted.
  WeakElementPointer this_element(this);
  if (ElementListener::InvokeElementInvokeEvent(this, ev)) {
    return true;
  }

  if (!this_element.get()) {
    return true;  // We got removed so we actually handled this event.
  }

  if (ev.type == EventType::kChanged) {
    InvalidateSkinStates();
    m_connection.SyncFromElement(this);
  }

  if (!this_element.get()) {
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
  }

  // Call OnEvent on this elements and travel up through its parents if not
  // handled.
  bool handled = false;
  Element* tmp = this;
  while (tmp && !(handled = tmp->OnEvent(ev))) {
    tmp = tmp->event_destination();
  }
  return handled;
}

void Element::StartLongClickTimer(bool touch) {
  StopLongClickTimer();
  m_long_click_timer = std::make_unique<LongClickTimer>(this, touch);
}

void Element::StopLongClickTimer() { m_long_click_timer.reset(); }

bool Element::InvokePointerDown(int x, int y, int click_count,
                                ModifierKeys modifierkeys, bool touch) {
  // If we have a captured element then the pointer event was handled since
  // focus is changed here.
  if (!captured_element) {
    SetCapturedElement(GetElementAt(x, y, true));
    SetHoveredElement(captured_element, touch);
    // captured_button = button;

    // Hide focus when we use the pointer, if it's not on the focused element.
    if (focused_element != captured_element) set_auto_focus_state(false);

    // Start long click timer. Only for touch events for now.
    if (touch && captured_element && captured_element->is_long_clickable()) {
      captured_element->StartLongClickTimer(touch);
    }

    // Get the closest parent form and bring it to the top.
    auto form = captured_element ? captured_element->parent_form() : nullptr;
    if (form) {
      form->Activate();
    }
  }
  if (captured_element) {
    // Check if there's any started scroller that should be stopped.
    Element* tmp = captured_element;
    while (tmp) {
      if (tmp->m_scroller && tmp->m_scroller->is_started()) {
        // When we touch down to stop a scroller, we don't
        // want the touch to end up causing a click.
        cancel_click = true;
        tmp->m_scroller->Stop();
        break;
      }
      tmp = tmp->parent();
    }

    // Focus the captured element or the closest focusable parent if it isn't
    // focusable.
    Element* focus_target = captured_element;
    while (focus_target) {
      if (focus_target->set_focus(FocusReason::kPointer)) {
        break;
      }
      focus_target = focus_target->m_parent;
    }
  }
  if (captured_element) {
    captured_element->ConvertFromRoot(&x, &y);
    pointer_move_element_x = pointer_down_element_x = x;
    pointer_move_element_y = pointer_down_element_y = y;
    Event ev(EventType::kPointerDown, x, y, touch, modifierkeys);
    ev.count = click_count;
    captured_element->InvokeEvent(std::move(ev));
    return true;
  }

  return false;
}

bool Element::InvokePointerUp(int x, int y, ModifierKeys modifierkeys,
                              bool touch) {
  // If we have a captured element then we have a focused element so the pointer
  // up event was handled
  if (captured_element) {
    captured_element->ConvertFromRoot(&x, &y);
    Event ev_up(EventType::kPointerUp, x, y, touch, modifierkeys);
    captured_element->InvokeEvent(std::move(ev_up));
    if (!cancel_click && captured_element &&
        captured_element->GetHitStatus(x, y) != HitStatus::kNoHit) {
      Event ev_click(EventType::kClick, x, y, touch, modifierkeys);
      captured_element->InvokeEvent(std::move(ev_click));
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
    Event ev_long_click(EventType::kLongClick, pointer_move_element_x,
                        pointer_move_element_y, touch, ModifierKeys::kNone);
    bool handled = captured_element->InvokeEvent(std::move(ev_long_click));
    if (!handled) {
      // Long click not handled so invoke a context menu event instead.
      Event ev_context_menu(EventType::kContextMenu, pointer_move_element_x,
                            pointer_move_element_y, touch, ModifierKeys::kNone);
      handled = captured_element->InvokeEvent(std::move(ev_context_menu));
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
    target->ConvertFromRoot(&x, &y);
    pointer_move_element_x = x;
    pointer_move_element_y = y;

    Event ev(EventType::kPointerMove, x, y, touch, modifierkeys);
    if (target->InvokeEvent(std::move(ev))) {
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
    auto scroller = captured_element->GetReadyScroller(dx != 0, dy != 0);
    if (!scroller) {
      return;
    }

    int old_translation_x = 0;
    int old_translation_y = 0;
    captured_element->scroll_root()->GetChildTranslation(&old_translation_x,
                                                         &old_translation_y);

    if (scroller->OnPan(dx + start_compensation_x, dy + start_compensation_y)) {
      // Scroll delta changed, so we are now panning!
      captured_element->m_packed.is_panning = true;
      cancel_click = true;

      // If the captured element (or its scroll root) has panned, we have to
      // compensate the pointer down coordinates so we won't accumulate the
      // difference the following pan.
      int new_translation_x = 0;
      int new_translation_y = 0;
      captured_element->scroll_root()->GetChildTranslation(&new_translation_x,
                                                           &new_translation_y);
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
  target->ConvertFromRoot(&x, &y);
  pointer_move_element_x = x;
  pointer_move_element_y = y;
  Event ev(EventType::kWheel, x, y, true, modifierkeys);
  ev.delta_x = delta_x;
  ev.delta_y = delta_y;
  target->InvokeEvent(std::move(ev));
  return true;
}

bool Element::InvokeKey(int key, SpecialKey special_key,
                        ModifierKeys modifierkeys, bool down) {
  bool handled = false;
  if (focused_element) {
    // Emulate a click on the focused element when pressing space or enter.
    if (!any(modifierkeys) && focused_element->is_click_by_key() &&
        focused_element->is_enabled() && !focused_element->is_dying() &&
        (special_key == SpecialKey::kEnter || key == ' ')) {
      // Set the pressed state while the key is down, if it didn't already have
      // the pressed state.
      static bool check_pressed_state = true;
      static bool had_pressed_state = false;
      if (down && check_pressed_state) {
        had_pressed_state =
            focused_element->has_state(Element::State::kPressed);
        check_pressed_state = false;
      }
      if (!down) {
        check_pressed_state = true;
      }

      if (!had_pressed_state) {
        focused_element->set_state(Element::State::kPressed, down);
        focused_element->m_packed.has_key_pressed_state = down;
      }

      // Invoke the click event.
      if (!down) {
        Event ev(EventType::kClick, m_rect.w / 2, m_rect.h / 2, true);
        focused_element->InvokeEvent(std::move(ev));
      }
      handled = true;
    } else {
      // Invoke the key event on the focused element.
      Event ev(down ? EventType::kKeyDown : EventType::kKeyUp);
      ev.key = key;
      ev.special_key = special_key;
      ev.modifierkeys = modifierkeys;
      handled = focused_element->InvokeEvent(std::move(ev));
    }
  }

  // Move focus between elements.
  if (down && !handled && special_key == SpecialKey::kTab) {
    handled = MoveFocus(!any(modifierkeys & ModifierKeys::kShift));

    // Show the focus when we move it by keyboard.
    if (handled) {
      set_auto_focus_state(true);
    }
  }
  return handled;
}

void Element::ReleaseCapture() {
  if (this == captured_element) {
    SetCapturedElement(nullptr);
  }
}

void Element::ConvertToRoot(int* x, int* y) const {
  const Element* tmp = this;
  while (tmp->m_parent) {
    *x += tmp->m_rect.x;
    *y += tmp->m_rect.y;
    tmp = tmp->m_parent;

    if (tmp) {
      int child_translation_x;
      int child_translation_y;
      tmp->GetChildTranslation(&child_translation_x, &child_translation_y);
      *x += child_translation_x;
      *y += child_translation_y;
    }
  }
}

void Element::ConvertFromRoot(int* x, int* y) const {
  const Element* tmp = this;
  while (tmp->m_parent) {
    *x -= tmp->m_rect.x;
    *y -= tmp->m_rect.y;
    tmp = tmp->m_parent;

    if (tmp) {
      int child_translation_x;
      int child_translation_y;
      tmp->GetChildTranslation(&child_translation_x, &child_translation_y);
      *x -= child_translation_x;
      *y -= child_translation_y;
    }
  }
}

// static
void Element::SetHoveredElement(Element* element, bool touch) {
  if (Element::hovered_element == element) {
    return;
  }
  if (element && element->has_state(Element::State::kDisabled)) {
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
  if (element && element->has_state(Element::State::kDisabled)) {
    return;
  }

  if (Element::captured_element) {
    // Stop panning when capture change (most likely changing to nullptr because
    // of InvokePointerUp).
    // Notify any active scroller so it may begin scrolling.
    if (auto scroller = Element::captured_element->FindStartedScroller()) {
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

bool Element::set_font_description(const FontDescription& font_desc) {
  if (m_font_desc == font_desc) return true;

  // Set the font description only if we have a matching font, or succeed
  // creating one.
  if (text::FontManager::get()->HasFontFace(font_desc)) {
    m_font_desc = font_desc;
  } else if (text::FontManager::get()->CreateFontFace(font_desc)) {
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
  for (Element* child = first_child(); child; child = child->GetNext()) {
    if (child->m_font_desc.font_face_id() == 0) {
      child->InvokeFontChanged();
    }
  }
}

FontDescription Element::computed_font_description() const {
  const Element* tmp = this;
  while (tmp) {
    if (tmp->m_font_desc.font_face_id() != 0) {
      return tmp->m_font_desc;
    }
    tmp = tmp->m_parent;
  }
  return text::FontManager::get()->default_font_description();
}

text::FontFace* Element::computed_font() const {
  return text::FontManager::get()->GetFontFace(computed_font_description());
}

using el::SkinCondition;
using el::SkinProperty;
using el::SkinTarget;

bool ElementSkinConditionContext::GetCondition(
    SkinTarget target, const SkinCondition::ConditionInfo& info) const {
  switch (target) {
    case SkinTarget::kThis:
      return GetCondition(m_element, info);
    case SkinTarget::kParent:
      return m_element->parent() && GetCondition(m_element->parent(), info);
    case SkinTarget::kAncestors: {
      Element* element = m_element->parent();
      while (element) {
        if (GetCondition(element, info)) {
          return true;
        }
        element = element->parent();
      }
    }
    case SkinTarget::kPrevSibling:
      return m_element->GetPrev() && GetCondition(m_element->GetPrev(), info);
    case SkinTarget::kNextSibling:
      return m_element->GetNext() && GetCondition(m_element->GetNext(), info);
  }
  return false;
}

bool ElementSkinConditionContext::GetCondition(
    Element* element, const SkinCondition::ConditionInfo& info) const {
  switch (info.prop) {
    case SkinProperty::kSkin:
      return element->background_skin() == info.value;
    case SkinProperty::kFormActive:
      if (auto form = element->parent_form()) {
        return form->is_active();
      }
      return false;
    case SkinProperty::kAxis:
      return TBID(element->axis() == Axis::kX ? "x" : "y") == info.value;
    case SkinProperty::kAlign:
      if (auto tc = util::SafeCast<elements::TabContainer>(element)) {
        TBID element_align;
        if (tc->alignment() == Align::kLeft) {
          element_align = TBIDC("left");
        } else if (tc->alignment() == Align::kTop) {
          element_align = TBIDC("top");
        } else if (tc->alignment() == Align::kRight) {
          element_align = TBIDC("right");
        } else if (tc->alignment() == Align::kBottom) {
          element_align = TBIDC("bottom");
        }
        return element_align == info.value;
      }
      return false;
    case SkinProperty::kId:
      return element->id() == info.value;
    case SkinProperty::kState:
      return !!(uint32_t(element->computed_state()) & info.value);
    case SkinProperty::kValue:
      return element->value() == static_cast<int>(info.value);
    case SkinProperty::kHover:
      return Element::hovered_element &&
             element->IsAncestorOf(Element::hovered_element);
    case SkinProperty::kCapture:
      return Element::captured_element &&
             element->IsAncestorOf(Element::captured_element);
    case SkinProperty::kFocus:
      return Element::focused_element &&
             element->IsAncestorOf(Element::focused_element);
    case SkinProperty::kCustom:
      return element->GetCustomSkinCondition(info);
  }
  return false;
}

}  // namespace el
