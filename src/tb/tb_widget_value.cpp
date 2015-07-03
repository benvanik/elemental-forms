/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widget_value.h"

#include "tb_widgets.h"

namespace tb {

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
  switch (m_value.GetType()) {
    case Value::Type::kString:
      m_value.SetString(source_element->GetText());
      break;
    case Value::Type::kNull:
    case Value::Type::kInt:
      m_value.SetInt(source_element->GetValue());
      break;
    case Value::Type::kFloat:
      // FIX: Value should use double instead of float?
      m_value.SetFloat(float(source_element->GetValueDouble()));
      break;
    default:
      assert(!"Unsupported value type!");
      break;
  }

  SyncToElements(source_element);
}

void ElementValue::SyncToElements(Element* exclude_element) {
  // FIX: Assign group to each value. Currently we only have one global group.
  g_value_group.InvokeOnValueChanged(this);

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
  switch (m_value.GetType()) {
    case Value::Type::kString:
      dst_element->SetText(m_value.GetString());
      break;
    case Value::Type::kNull:
    case Value::Type::kInt:
      dst_element->SetValue(m_value.GetInt());
      break;
    case Value::Type::kFloat:
      // FIX: Value should use double instead of float?
      dst_element->SetValueDouble(m_value.GetFloat());
      break;
    default:
      assert(!"Unsupported value type!");
      break;
  }
  m_syncing = false;
}

void ElementValue::SetInt(int value) {
  m_value.SetInt(value);
  SyncToElements(nullptr);
}

void ElementValue::SetText(const char* text) {
  m_value.SetString(text, Value::Set::kNewCopy);
  SyncToElements(nullptr);
}

void ElementValue::SetDouble(double value) {
  // FIX: Value should use double instead of float?
  m_value.SetFloat(float(value));
  SyncToElements(nullptr);
}

/*extern*/ ValueGroup g_value_group;

ElementValue* ValueGroup::CreateValueIfNeeded(const TBID& name,
                                              Value::Type type) {
  if (ElementValue* val = GetValue(name)) {
    return val;
  }
  ElementValue* val = new ElementValue(name, type);
  m_values.Add(name, val);
  return val;
}

void ValueGroup::InvokeOnValueChanged(const ElementValue* value) {
  auto iter = m_listeners.IterateForward();
  while (ValueGroupListener* listener = iter.GetAndStep()) {
    listener->OnValueChanged(this, value);
  }
}

}  // namespace tb
