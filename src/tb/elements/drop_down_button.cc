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
  SetSource(&m_default_source);
  SetSkinBg(TBIDC("DropDownButton"), InvokeInfo::kNoCallbacks);
  m_arrow.SetSkinBg(TBIDC("DropDownButton.arrow"), InvokeInfo::kNoCallbacks);
  GetContentRoot()->AddChild(&m_arrow);
}

DropDownButton::~DropDownButton() {
  GetContentRoot()->RemoveChild(&m_arrow);
  SetSource(nullptr);
  CloseWindow();
}

void DropDownButton::OnInflate(const parsing::InflateInfo& info) {
  // Read items (if there is any) into the default source.
  GenericStringItemSource::ReadItemNodes(info.node, GetDefaultSource());
  Element::OnInflate(info);
}

void DropDownButton::OnSourceChanged() {
  m_value = -1;
  if (m_source && m_source->size()) {
    SetValue(0);
  }
}

void DropDownButton::OnItemChanged(size_t index) {}

void DropDownButton::SetValue(int value) {
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

TBID DropDownButton::GetSelectedItemID() {
  if (m_source && m_value >= 0 && m_value < m_source->size()) {
    return m_source->GetItemID(m_value);
  }
  return TBID();
}

void DropDownButton::OpenWindow() {
  if (!m_source || !m_source->size() || m_window_pointer.Get()) {
    return;
  }

  MenuWindow* window = new MenuWindow(this, TBIDC("DropDownButton.window"));
  m_window_pointer.Set(window);
  window->SetSkinBg(TBIDC("DropDownButton.window"));
  window->Show(m_source, PopupAlignment(), GetValue());
}

void DropDownButton::CloseWindow() {
  if (MenuWindow* window = GetMenuIfOpen()) {
    window->Close();
  }
}

MenuWindow* DropDownButton::GetMenuIfOpen() const {
  return util::SafeCast<MenuWindow>(m_window_pointer.Get());
}

bool DropDownButton::OnEvent(const ElementEvent& ev) {
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
  } else if (ev.target->id() == TBIDC("DropDownButton.window") &&
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

}  // namespace elements
}  // namespace tb
