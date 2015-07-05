/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>

#include "tb_menu_window.h"
#include "tb_select.h"
#include "tb_widgets_listener.h"

#include "tb/parsing/element_inflater.h"
#include "tb/util/string.h"
#include "tb/util/string_builder.h"
#include "tb/util/string_table.h"

namespace tb {

void SelectList::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(SelectList, Value::Type::kInt, ElementZ::kTop);
}

SelectList::SelectList() : m_header_lng_string_id(TBIDC("SelectList.header")) {
  SetSource(&m_default_source);
  SetIsFocusable(true);
  SetSkinBg(TBIDC("SelectList"), InvokeInfo::kNoCallbacks);
  m_container.SetGravity(Gravity::kAll);
  m_container.set_rect(GetPaddingRect());
  AddChild(&m_container);
  m_layout.SetGravity(Gravity::kAll);
  m_layout.SetAxis(Axis::kY);
  m_layout.SetSpacing(0);
  m_layout.SetLayoutPosition(LayoutPosition::kLeftTop);
  m_layout.SetLayoutDistributionPosition(LayoutDistributionPosition::kLeftTop);
  m_layout.SetLayoutSize(LayoutSize::kAvailable);
  m_container.GetContentRoot()->AddChild(&m_layout);
  m_container.SetScrollMode(ScrollMode::kAutoY);
  m_container.SetAdaptContentSize(true);
}

SelectList::~SelectList() {
  m_container.GetContentRoot()->RemoveChild(&m_layout);
  RemoveChild(&m_container);
  SetSource(nullptr);
}

void SelectList::OnInflate(const parsing::InflateInfo& info) {
  // Read items (if there is any) into the default source.
  Element::ReadItemNodes(info.node, GetDefaultSource());
  Element::OnInflate(info);
}

void SelectList::OnSourceChanged() { InvalidateList(); }

void SelectList::OnItemChanged(size_t index) {
  if (m_list_is_invalid) {
    // We're updating all elements soon.
    return;
  }

  Element* old_element = GetItemElement(index);
  if (!old_element) {
    // We don't have this element so we have nothing to update.
    return;
  }

  // Replace the old element representing the item, with a new one. Preserve its
  // state.
  auto old_state = old_element->GetStateRaw();

  if (Element* element = CreateAndAddItemAfter(index, old_element)) {
    element->SetStateRaw(old_state);
  }

  old_element->GetParent()->RemoveChild(old_element);
  delete old_element;
}

void SelectList::OnItemAdded(size_t index) {
  if (m_list_is_invalid) {
    // We're updating all elements soon.
    return;
  }

  // Sorting, filtering etc. makes it messy to handle dynamic addition of items.
  // Resort to invalidate the entire list (may even be faster anyway)
  InvalidateList();
}

void SelectList::OnItemRemoved(size_t index) {
  if (m_list_is_invalid) {
    // We're updating all elements soon.
    return;
  }

  // Sorting, filtering etc. makes it messy to handle dynamic addition of items.
  // Resort to invalidate the entire list (may even be faster anyway)
  InvalidateList();
}

void SelectList::OnAllItemsRemoved() {
  InvalidateList();
  m_value = -1;
}

void SelectList::SetFilter(const char* filter) {
  std::string new_filter;
  if (filter && *filter) {
    new_filter = filter;
  }
  if (m_filter.compare(new_filter) == 0) {
    return;
  }
  m_filter = new_filter;
  InvalidateList();
}

void SelectList::SetHeaderString(const TBID& id) {
  if (m_header_lng_string_id == id) return;
  m_header_lng_string_id = id;
  InvalidateList();
}

void SelectList::InvalidateList() {
  if (m_list_is_invalid) return;
  m_list_is_invalid = true;
  Invalidate();
}

void SelectList::ValidateList() {
  if (!m_list_is_invalid) return;
  m_list_is_invalid = false;
  // FIX: Could delete and create only the changed items (faster filter change).

  // Remove old items.
  while (Element* child = m_layout.GetContentRoot()->GetFirstChild()) {
    child->GetParent()->RemoveChild(child);
    delete child;
  }
  if (!m_source || !m_source->size()) {
    return;
  }

  // Create a sorted list of the items we should include using the current
  // filter.
  util::StringBuilder sort_buf(m_source->size() * sizeof(int));
  int* sorted_index = (int*)sort_buf.GetData();

  // Populate the sorted index list.
  int num_sorted_items = 0;
  for (size_t i = 0; i < m_source->size(); ++i) {
    if (m_filter.empty() || m_source->Filter(i, m_filter)) {
      sorted_index[num_sorted_items++] = int(i);
    }
  }

  // Sort.
  if (m_source->GetSort() != Sort::kNone) {
    std::sort(&sorted_index[0], &sorted_index[num_sorted_items],
              [&](const int a, const int b) {
                int value = strcmp(m_source->GetItemString(a),
                                   m_source->GetItemString(b));
                return m_source->GetSort() == Sort::kDescending ? value > 0
                                                                : value < 0;
              });
  }

  // Show header if we only show a subset of all items.
  if (!m_filter.empty()) {
    Element* element = new Label();
    auto fmt = util::GetString(m_header_lng_string_id);
    element->SetText(tb::util::format_string(fmt.c_str(), num_sorted_items,
                                             m_source->size()));
    element->SetSkinBg(TBIDC("SelectList.header"));
    element->SetState(Element::State::kDisabled, true);
    element->SetGravity(Gravity::kAll);
    element->data.set_integer(-1);
    m_layout.GetContentRoot()->AddChild(element);
  }

  // Create new items.
  for (int i = 0; i < num_sorted_items; ++i) {
    CreateAndAddItemAfter(sorted_index[i], nullptr);
  }

  SelectItem(m_value, true);

  // FIX: Should not scroll just because we update the list. Only automatically
  // first time!
  m_scroll_to_current = true;
}

Element* SelectList::CreateAndAddItemAfter(size_t index, Element* reference) {
  if (Element* element = m_source->CreateItemElement(index, this)) {
    // Use item data as element to index lookup.
    element->data.set_integer(int(index));
    m_layout.GetContentRoot()->AddChildRelative(element, ElementZRel::kAfter,
                                                reference);
    return element;
  }
  return nullptr;
}

void SelectList::SetValue(int value) {
  if (value == m_value) return;

  SelectItem(m_value, false);
  m_value = value;
  SelectItem(m_value, true);
  ScrollToSelectedItem();

  ElementEvent ev(EventType::kChanged);
  if (Element* element = GetItemElement(m_value)) {
    ev.ref_id = element->id();
  }
  InvokeEvent(ev);
}

TBID SelectList::GetSelectedItemID() {
  if (m_source && m_value >= 0 && m_value < m_source->size()) {
    return m_source->GetItemID(m_value);
  }
  return TBID();
}

void SelectList::SelectItem(size_t index, bool selected) {
  if (Element* element = GetItemElement(index)) {
    element->SetState(Element::State::kSelected, selected);
  }
}

Element* SelectList::GetItemElement(size_t index) {
  if (index == -1) return nullptr;
  for (Element* tmp = m_layout.GetContentRoot()->GetFirstChild(); tmp;
       tmp = tmp->GetNext()) {
    if (tmp->data.as_integer() == index) return tmp;
  }
  return nullptr;
}

void SelectList::ScrollToSelectedItem() {
  if (m_list_is_invalid) {
    m_scroll_to_current = true;
    return;
  }
  m_scroll_to_current = false;
  if (Element* element = GetItemElement(m_value)) {
    m_container.ScrollIntoView(element->rect());
  } else {
    m_container.ScrollTo(0, 0);
  }
}

void SelectList::OnSkinChanged() { m_container.set_rect(GetPaddingRect()); }

void SelectList::OnProcess() { ValidateList(); }

void SelectList::OnProcessAfterChildren() {
  if (m_scroll_to_current) {
    ScrollToSelectedItem();
  }
}

bool SelectList::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick &&
      ev.target->GetParent() == m_layout.GetContentRoot()) {
    // SetValue (EventType::kChanged) might cause something to delete this (f.ex
    // closing the dropdown menu. We want to sent another event, so ensure we're
    // still around.
    WeakElementPointer this_element(this);

    size_t index = ev.target->data.as_integer();
    SetValue(int(index));

    // If we're still around, invoke the click event too.
    if (this_element.Get()) {
      SelectList* target_list = this;
      // If the parent window is a MenuWindow, we will iterate up the event
      // destination chain to find the top MenuWindow and invoke the event
      // there.
      // That way events in submenus will reach the caller properly, and seem
      // like it was invoked on the top menu.
      Window* window = GetParentWindow();
      while (MenuWindow* menu_win = util::SafeCast<MenuWindow>(window)) {
        target_list = menu_win->GetList();
        window = menu_win->GetEventDestination()->GetParentWindow();
      }

      // Invoke the click event on the target list.
      ElementEvent ev(EventType::kClick);
      if (Element* element = GetItemElement(m_value)) {
        ev.ref_id = element->id();
      }
      target_list->InvokeEvent(ev);
    }
    return true;
  } else if (ev.type == EventType::kKeyDown) {
    if (ChangeValue(ev.special_key)) {
      return true;
    }

    // Give the scroll container a chance to handle the key so it may
    // scroll. This matters if the list itself is focused instead of
    // some child view of any select item (since that would have passed
    // the container already)
    if (GetScrollContainer()->OnEvent(ev)) {
      return true;
    }
  }
  return false;
}

bool SelectList::ChangeValue(SpecialKey key) {
  if (!m_source || !m_layout.GetContentRoot()->GetFirstChild()) {
    return false;
  }

  bool forward;
  if (key == SpecialKey::kHome || key == SpecialKey::kDown) {
    forward = true;
  } else if (key == SpecialKey::kEnd || key == SpecialKey::kUp) {
    forward = false;
  } else {
    return false;
  }

  Element* item_root = m_layout.GetContentRoot();
  Element* current = GetItemElement(m_value);
  Element* origin = nullptr;
  if (key == SpecialKey::kHome || (!current && key == SpecialKey::kDown)) {
    current = item_root->GetFirstChild();
  } else if (key == SpecialKey::kEnd || (!current && key == SpecialKey::kUp)) {
    current = item_root->GetLastChild();
  } else {
    origin = current;
  }

  while (current) {
    if (current != origin && !current->GetDisabled()) {
      break;
    }
    current = forward ? current->GetNext() : current->GetPrev();
  }
  // Select and focus what we found.
  if (current) {
    SetValue(current->data.as_integer());
    return true;
  }
  return false;
}

void SelectDropdown::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(SelectDropdown, Value::Type::kInt,
                               ElementZ::kTop);
}

SelectDropdown::SelectDropdown() {
  SetSource(&m_default_source);
  SetSkinBg(TBIDC("SelectDropdown"), InvokeInfo::kNoCallbacks);
  m_arrow.SetSkinBg(TBIDC("SelectDropdown.arrow"), InvokeInfo::kNoCallbacks);
  GetContentRoot()->AddChild(&m_arrow);
}

SelectDropdown::~SelectDropdown() {
  GetContentRoot()->RemoveChild(&m_arrow);
  SetSource(nullptr);
  CloseWindow();
}

void SelectDropdown::OnInflate(const parsing::InflateInfo& info) {
  // Read items (if there is any) into the default source.
  Element::ReadItemNodes(info.node, GetDefaultSource());
  Element::OnInflate(info);
}

void SelectDropdown::OnSourceChanged() {
  m_value = -1;
  if (m_source && m_source->size()) {
    SetValue(0);
  }
}

void SelectDropdown::OnItemChanged(size_t index) {}

void SelectDropdown::SetValue(int value) {
  if (value == m_value || !m_source) return;
  m_value = value;

  if (m_value < 0) {
    SetText("");
  } else if (m_value < m_source->size()) {
    SetText(m_source->GetItemString(m_value));
  }

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);
}

TBID SelectDropdown::GetSelectedItemID() {
  if (m_source && m_value >= 0 && m_value < m_source->size()) {
    return m_source->GetItemID(m_value);
  }
  return TBID();
}

void SelectDropdown::OpenWindow() {
  if (!m_source || !m_source->size() || m_window_pointer.Get()) {
    return;
  }

  MenuWindow* window = new MenuWindow(this, TBIDC("SelectDropdown.window"));
  m_window_pointer.Set(window);
  window->SetSkinBg(TBIDC("SelectDropdown.window"));
  window->Show(m_source, PopupAlignment(), GetValue());
}

void SelectDropdown::CloseWindow() {
  if (MenuWindow* window = GetMenuIfOpen()) {
    window->Close();
  }
}

MenuWindow* SelectDropdown::GetMenuIfOpen() const {
  return util::SafeCast<MenuWindow>(m_window_pointer.Get());
}

bool SelectDropdown::OnEvent(const ElementEvent& ev) {
  if (ev.target == this && ev.type == EventType::kClick) {
    // Open the menu, or set the value and close it if already open (this will
    // happen when clicking by keyboard since that will call click on this
    // button).
    if (MenuWindow* menu_window = GetMenuIfOpen()) {
      WeakElementPointer tmp(this);
      int value = menu_window->GetList()->GetValue();
      menu_window->Die();
      if (tmp.Get()) {
        SetValue(value);
      }
    } else {
      OpenWindow();
    }
    return true;
  } else if (ev.target->id() == TBIDC("SelectDropdown.window") &&
             ev.type == EventType::kClick) {
    // Set the value of the clicked item.
    if (MenuWindow* menu_window = GetMenuIfOpen()) {
      SetValue(menu_window->GetList()->GetValue());
    }
    return true;
  } else if (ev.target == this && m_source && ev.IsKeyEvent()) {
    if (MenuWindow* menu_window = GetMenuIfOpen()) {
      // Redirect the key strokes to the list.
      ElementEvent redirected_ev(ev);
      return menu_window->GetList()->InvokeEvent(redirected_ev);
    }
  }
  return false;
}

}  // namespace tb
