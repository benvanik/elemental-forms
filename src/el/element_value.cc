/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/element.h"
#include "el/element_value.h"

namespace el {

ElementValueGroup ElementValueGroup::value_group_singleton_;

void ElementValueConnection::Connect(ElementValue* value, Element* element) {
  Disconnect();
  m_element = element;
  m_value = value;
  m_value->m_connections.AddLast(this);
  m_value->SyncToElement(m_element);
}

void ElementValueConnection::Disconnect() {
  if (m_value) {
    m_value->m_connections.Remove(this);
  }
  m_value = nullptr;
  m_element = nullptr;
}

void ElementValueConnection::SyncFromElement(Element* source_element) {
  if (m_value) {
    m_value->SetFromElement(source_element);
  }
}

ElementValue::ElementValue(const TBID& name, Value::Type type)
    : m_name(name), m_value(type) {}

ElementValue::~ElementValue() {
  while (m_connections.GetFirst()) {
    m_connections.GetFirst()->Disconnect();
  }
}

void ElementValue::SetFromElement(Element* source_element) {
  if (m_syncing) {
    // We ended up here because syncing is in progress.
    return;
  }

  // Get the value in the format.
  switch (m_value.type()) {
    case Value::Type::kString:
      m_value.set_string(source_element->text());
      break;
    case Value::Type::kNull:
    case Value::Type::kInt:
      m_value.set_integer(source_element->value());
      break;
    case Value::Type::kFloat:
      // FIX: Value should use double instead of float?
      m_value.set_float(float(source_element->double_value()));
      break;
    default:
      assert(!"Unsupported value type!");
      break;
  }

  SyncToElements(source_element);
}

void ElementValue::SyncToElements(Element* exclude_element) {
  // FIX: Assign group to each value. Currently we only have one global group.
  ElementValueGroup::get()->InvokeOnValueChanged(this);

  auto iter = m_connections.IterateForward();
  while (ElementValueConnection* connection = iter.GetAndStep()) {
    if (connection->m_element != exclude_element) {
      SyncToElement(connection->m_element);
    }
  }
}

void ElementValue::SyncToElement(Element* dst_element) {
  if (m_syncing) {
    // We ended up here because syncing is in progress.
    return;
  }

  m_syncing = true;
  switch (m_value.type()) {
    case Value::Type::kString:
      dst_element->set_text(m_value.as_string());
      break;
    case Value::Type::kNull:
    case Value::Type::kInt:
      dst_element->set_value(m_value.as_integer());
      break;
    case Value::Type::kFloat:
      // FIX: Value should use double instead of float?
      dst_element->set_double_value(m_value.as_float());
      break;
    default:
      assert(!"Unsupported value type!");
      break;
  }
  m_syncing = false;
}

void ElementValue::set_integer(int value) {
  m_value.set_integer(value);
  SyncToElements(nullptr);
}

void ElementValue::set_text(const char* text) {
  m_value.set_string(text, Value::Set::kNewCopy);
  SyncToElements(nullptr);
}

void ElementValue::set_double(double value) {
  // FIX: Value should use double instead of float?
  m_value.set_float(float(value));
  SyncToElements(nullptr);
}

ElementValue* ElementValueGroup::CreateValueIfNeeded(const TBID& name,
                                                     Value::Type type) {
  if (auto existing_value = GetValue(name)) {
    return existing_value;
  }
  auto new_value = std::make_unique<ElementValue>(name, type);
  auto new_value_ptr = new_value.get();
  m_values.emplace(name, std::move(new_value));
  return new_value_ptr;
}

ElementValue* ElementValueGroup::GetValue(const TBID& name) const {
  auto it = m_values.find(name);
  return it != m_values.end() ? it->second.get() : nullptr;
}

void ElementValueGroup::InvokeOnValueChanged(const ElementValue* value) {
  auto iter = m_listeners.IterateForward();
  while (ElementValueGroupListener* listener = iter.GetAndStep()) {
    listener->OnValueChanged(this, value);
  }
}

}  // namespace el
