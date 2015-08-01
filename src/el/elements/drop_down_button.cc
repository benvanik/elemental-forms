/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/drop_down_button.h"
#include "el/elements/menu_form.h"
#include "el/parsing/element_inflater.h"
#include "el/util/string.h"
#include "el/util/string_builder.h"
#include "el/util/string_table.h"

namespace el {
namespace elements {

void DropDownButton::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(DropDownButton, Value::Type::kInt,
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
  CloseForm();
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

  Event ev(EventType::kChanged);
  InvokeEvent(ev);
}

TBID DropDownButton::selected_item_id() {
  if (m_source && m_value >= 0 && m_value < m_source->size()) {
    return m_source->GetItemId(m_value);
  }
  return TBID();
}

void DropDownButton::OpenForm() {
  if (!m_source || !m_source->size() || m_form_pointer.get()) {
    return;
  }

  MenuForm* form = new MenuForm(this, TBIDC("DropDownButton.form"));
  m_form_pointer.reset(form);
  form->set_background_skin(TBIDC("DropDownButton.form"));
  form->Show(m_source, PopupAlignment(), value());
}

void DropDownButton::CloseForm() {
  if (MenuForm* form = menu_if_open()) {
    form->Close();
  }
}

MenuForm* DropDownButton::menu_if_open() const {
  return util::SafeCast<MenuForm>(m_form_pointer.get());
}

bool DropDownButton::OnEvent(const Event& ev) {
  if (ev.target == this && ev.type == EventType::kClick) {
    // Open the menu, or set the value and close it if already open (this will
    // happen when clicking by keyboard since that will call click on this
    // button).
    if (MenuForm* menu_form = menu_if_open()) {
      WeakElementPointer tmp(this);
      int value = menu_form->list_box()->value();
      menu_form->Die();
      if (tmp.get()) {
        set_value(value);
      }
    } else {
      OpenForm();
    }
    return true;
  } else if (ev.target->id() == TBIDC("DropDownButton.form") &&
             ev.type == EventType::kClick) {
    // Set the value of the clicked item.
    if (MenuForm* menu_form = menu_if_open()) {
      set_value(menu_form->list_box()->value());
    }
    return true;
  } else if (ev.target == this && m_source && ev.is_key_event()) {
    if (MenuForm* menu_form = menu_if_open()) {
      // Redirect the key strokes to the list.
      Event redirected_ev(ev);
      return menu_form->list_box()->InvokeEvent(redirected_ev);
    }
  }
  return Element::OnEvent(ev);
}

}  // namespace elements
}  // namespace el
