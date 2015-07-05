/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/drop_down_button.h"
#include "tb/elements/menu_window.h"
#include "tb/parsing/element_inflater.h"
#include "tb/util/string.h"
#include "tb/util/string_builder.h"
#include "tb/util/string_table.h"

namespace tb {
namespace elements {

void DropDownButton::RegisterInflater() {
  TB_REGISTER_ELEMENT_INFLATER(DropDownButton, Value::Type::kInt,
                               ElementZ::kTop);
}

DropDownButton::DropDownButton() {
  set_source(&m_default_source);
  set_background_skin(TBIDC("DropDownButton"), InvokeInfo::kNoCallbacks);
  m_arrow.set_background_skin(TBIDC("DropDownButton.arrow"),
                              InvokeInfo::kNoCallbacks);
  content_root()->AddChild(&m_arrow);
}

DropDownButton::~DropDownButton() {
  content_root()->RemoveChild(&m_arrow);
  set_source(nullptr);
  CloseWindow();
}

void DropDownButton::OnInflate(const parsing::InflateInfo& info) {
  // Read items (if there is any) into the default source.
  GenericStringItemSource::ReadItemNodes(info.node, default_source());
  Element::OnInflate(info);
}

void DropDownButton::OnSourceChanged() {
  m_value = -1;
  if (m_source && m_source->size()) {
    set_value(0);
  }
}

void DropDownButton::OnItemChanged(size_t index) {}

void DropDownButton::set_value(int value) {
  if (value == m_value || !m_source) return;
  m_value = value;

  if (m_value < 0) {
    set_text("");
  } else if (m_value < m_source->size()) {
    set_text(m_source->GetItemString(m_value));
  }

  ElementEvent ev(EventType::kChanged);
  InvokeEvent(ev);
}

TBID DropDownButton::selected_item_id() {
  if (m_source && m_value >= 0 && m_value < m_source->size()) {
    return m_source->GetItemId(m_value);
  }
  return TBID();
}

void DropDownButton::OpenWindow() {
  if (!m_source || !m_source->size() || m_window_pointer.get()) {
    return;
  }

  MenuWindow* window = new MenuWindow(this, TBIDC("DropDownButton.window"));
  m_window_pointer.reset(window);
  window->set_background_skin(TBIDC("DropDownButton.window"));
  window->Show(m_source, PopupAlignment(), value());
}

void DropDownButton::CloseWindow() {
  if (MenuWindow* window = menu_if_open()) {
    window->Close();
  }
}

MenuWindow* DropDownButton::menu_if_open() const {
  return util::SafeCast<MenuWindow>(m_window_pointer.get());
}

bool DropDownButton::OnEvent(const ElementEvent& ev) {
  if (ev.target == this && ev.type == EventType::kClick) {
    // Open the menu, or set the value and close it if already open (this will
    // happen when clicking by keyboard since that will call click on this
    // button).
    if (MenuWindow* menu_window = menu_if_open()) {
      WeakElementPointer tmp(this);
      int value = menu_window->list_box()->value();
      menu_window->Die();
      if (tmp.get()) {
        set_value(value);
      }
    } else {
      OpenWindow();
    }
    return true;
  } else if (ev.target->id() == TBIDC("DropDownButton.window") &&
             ev.type == EventType::kClick) {
    // Set the value of the clicked item.
    if (MenuWindow* menu_window = menu_if_open()) {
      set_value(menu_window->list_box()->value());
    }
    return true;
  } else if (ev.target == this && m_source && ev.is_key_event()) {
    if (MenuWindow* menu_window = menu_if_open()) {
      // Redirect the key strokes to the list.
      ElementEvent redirected_ev(ev);
      return menu_window->list_box()->InvokeEvent(redirected_ev);
    }
  }
  return false;
}

}  // namespace elements
}  // namespace tb
