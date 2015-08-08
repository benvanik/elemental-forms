/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/form.h"
#include "el/elements/label.h"
#include "el/elements/list_box.h"
#include "el/elements/menu_form.h"
#include "el/parsing/element_inflater.h"
#include "el/util/string.h"
#include "el/util/string_table.h"

namespace el {
namespace elements {

void ListBox::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(ListBox, Value::Type::kInt, ElementZ::kTop);
}

ListBox::ListBox() : m_header_lng_string_id(TBIDC("ListBox.header")) {
  set_source(&m_default_source);
  set_focusable(true);
  set_background_skin(TBIDC("ListBox"), InvokeInfo::kNoCallbacks);
  m_container.set_gravity(Gravity::kAll);
  m_container.set_rect(padding_rect());
  AddChild(&m_container);
  m_layout.set_gravity(Gravity::kAll);
  m_layout.set_axis(Axis::kY);
  m_layout.set_spacing(0);
  m_layout.set_layout_position(LayoutPosition::kLeftTop);
  m_layout.set_layout_distribution_position(
      LayoutDistributionPosition::kLeftTop);
  m_layout.set_layout_size(LayoutSize::kAvailable);
  m_container.content_root()->AddChild(&m_layout);
  m_container.set_scroll_mode(ScrollMode::kAutoY);
  m_container.set_adapt_content_size(true);
}

ListBox::~ListBox() {
  m_container.content_root()->RemoveChild(&m_layout);
  RemoveChild(&m_container);
  set_source(nullptr);
}

void ListBox::OnInflate(const parsing::InflateInfo& info) {
  // Read items (if there is any) into the default source.
  GenericStringItemSource::ReadItemNodes(info.node, default_source());
  Element::OnInflate(info);
}

void ListBox::OnSourceChanged() { InvalidateList(); }

void ListBox::OnItemChanged(size_t index) {
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
  auto old_state = old_element->state_raw();

  if (Element* element = CreateAndAddItemAfter(index, old_element)) {
    element->set_state_raw(old_state);
  }

  old_element->parent()->RemoveChild(old_element);
  delete old_element;
}

void ListBox::OnItemAdded(size_t index) {
  if (m_list_is_invalid) {
    // We're updating all elements soon.
    return;
  }

  // Sorting, filtering etc. makes it messy to handle dynamic addition of items.
  // Resort to invalidate the entire list (may even be faster anyway)
  InvalidateList();
}

void ListBox::OnItemRemoved(size_t index) {
  if (m_list_is_invalid) {
    // We're updating all elements soon.
    return;
  }

  // Sorting, filtering etc. makes it messy to handle dynamic addition of items.
  // Resort to invalidate the entire list (may even be faster anyway)
  InvalidateList();
}

void ListBox::OnAllItemsRemoved() {
  InvalidateList();
  m_value = -1;
}

void ListBox::set_filter(const char* filter) {
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

void ListBox::set_header_string(const TBID& id) {
  if (m_header_lng_string_id == id) return;
  m_header_lng_string_id = id;
  InvalidateList();
}

void ListBox::InvalidateList() {
  if (m_list_is_invalid) return;
  m_list_is_invalid = true;
  Invalidate();
}

void ListBox::ValidateList() {
  if (!m_list_is_invalid) return;
  m_list_is_invalid = false;
  // FIX: Could delete and create only the changed items (faster filter change).

  // Remove old items.
  while (Element* child = m_layout.content_root()->first_child()) {
    child->parent()->RemoveChild(child);
    delete child;
  }
  if (!m_source || !m_source->size()) {
    return;
  }

  // Create a sorted list of the items we should include using the current
  // filter.
  std::vector<int> sorted_index;
  sorted_index.resize(m_source->size());

  // Populate the sorted index list.
  int num_sorted_items = 0;
  for (size_t i = 0; i < m_source->size(); ++i) {
    if (m_filter.empty() || m_source->Filter(i, m_filter)) {
      sorted_index[num_sorted_items++] = int(i);
    }
  }

  // Sort.
  if (m_source->sort() != Sort::kNone) {
    std::sort(
        &sorted_index[0], &sorted_index[0] + num_sorted_items,
        [&](const int a, const int b) {
          int value =
              strcmp(m_source->GetItemString(a), m_source->GetItemString(b));
          return m_source->sort() == Sort::kDescending ? value > 0 : value < 0;
        });
  }

  // Show header if we only show a subset of all items.
  if (!m_filter.empty()) {
    Element* element = new Label();
    auto fmt = util::GetString(m_header_lng_string_id);
    element->set_text_format(fmt.c_str(), num_sorted_items, m_source->size());
    element->set_background_skin(TBIDC("ListBox.header"));
    element->set_state(Element::State::kDisabled, true);
    element->set_gravity(Gravity::kAll);
    element->data.set_integer(-1);
    m_layout.content_root()->AddChild(element);
  }

  // Create new items.
  for (int i = 0; i < num_sorted_items; ++i) {
    CreateAndAddItemAfter(sorted_index[i], nullptr);
  }

  ListItem(m_value, true);

  // FIX: Should not scroll just because we update the list. Only automatically
  // first time!
  m_scroll_to_current = true;
}

Element* ListBox::CreateAndAddItemAfter(size_t index, Element* reference) {
  if (Element* element = m_source->CreateItemElement(index, this)) {
    // Use item data as element to index lookup.
    element->data.set_integer(int(index));
    m_layout.content_root()->AddChildRelative(element, ElementZRel::kAfter,
                                              reference);
    return element;
  }
  return nullptr;
}

void ListBox::set_value(int value) {
  if (value == m_value) return;

  ListItem(m_value, false);
  m_value = value;
  ListItem(m_value, true);
  ScrollToSelectedItem();

  Event ev(EventType::kChanged);
  if (Element* element = GetItemElement(m_value)) {
    ev.ref_id = element->id();
  }
  InvokeEvent(ev);
}

TBID ListBox::selected_item_id() {
  if (m_source && m_value >= 0 && m_value < m_source->size()) {
    return m_source->GetItemId(m_value);
  }
  return TBID();
}

void ListBox::ListItem(size_t index, bool selected) {
  if (Element* element = GetItemElement(index)) {
    element->set_state(Element::State::kSelected, selected);
  }
}

Element* ListBox::GetItemElement(size_t index) {
  if (index == -1) return nullptr;
  for (Element* tmp = m_layout.content_root()->first_child(); tmp;
       tmp = tmp->GetNext()) {
    if (tmp->data.as_integer() == index) return tmp;
  }
  return nullptr;
}

void ListBox::ScrollToSelectedItem() {
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

void ListBox::OnSkinChanged() { m_container.set_rect(padding_rect()); }

void ListBox::OnProcess() { ValidateList(); }

void ListBox::OnProcessAfterChildren() {
  if (m_scroll_to_current) {
    ScrollToSelectedItem();
  }
}

bool ListBox::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick &&
      ev.target->parent() == m_layout.content_root()) {
    // SetValue (EventType::kChanged) might cause something to delete this (f.ex
    // closing the dropdown menu. We want to sent another event, so ensure we're
    // still around.
    WeakElementPointer this_element(this);

    size_t index = ev.target->data.as_integer();
    set_value(int(index));

    // If we're still around, invoke the click event too.
    if (this_element.get()) {
      ListBox* target_list = this;
      // If the parent form is a MenuForm, we will iterate up the event
      // destination chain to find the top MenuForm and invoke the event
      // there.
      // That way events in submenus will reach the caller properly, and seem
      // like it was invoked on the top menu.
      Form* form = parent_form();
      while (auto menu_win = util::SafeCast<MenuForm>(form)) {
        target_list = menu_win->list_box();
        form = menu_win->event_destination()->parent_form();
      }

      // Invoke the click event on the target list.
      Event invoke_ev(EventType::kClick);
      if (Element* element = GetItemElement(m_value)) {
        invoke_ev.ref_id = element->id();
      }
      target_list->InvokeEvent(invoke_ev);
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
    if (scroll_container()->OnEvent(ev)) {
      return true;
    }
  }
  return Element::OnEvent(ev);
}

bool ListBox::ChangeValue(SpecialKey key) {
  if (!m_source || !m_layout.content_root()->first_child()) {
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

  Element* item_root = m_layout.content_root();
  Element* current = GetItemElement(m_value);
  Element* origin = nullptr;
  if (key == SpecialKey::kHome || (!current && key == SpecialKey::kDown)) {
    current = item_root->first_child();
  } else if (key == SpecialKey::kEnd || (!current && key == SpecialKey::kUp)) {
    current = item_root->last_child();
  } else {
    origin = current;
  }

  while (current) {
    if (current != origin && current->is_enabled()) {
      break;
    }
    current = forward ? current->GetNext() : current->GetPrev();
  }
  // Select and focus what we found.
  if (current) {
    set_value(current->data.as_integer());
    return true;
  }
  return false;
}

}  // namespace elements
}  // namespace el
