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

void WidgetValueConnection::Connect(WidgetValue* value, Widget* widget) {
  Disconnect();
  m_widget = widget;
  m_value = value;
  m_value->m_connections.AddLast(this);
  m_value->SyncToWidget(m_widget);
}

void WidgetValueConnection::Disconnect() {
  if (m_value) {
    m_value->m_connections.Remove(this);
  }
  m_value = nullptr;
  m_widget = nullptr;
}

void WidgetValueConnection::SyncFromWidget(Widget* source_widget) {
  if (m_value) {
    m_value->SetFromWidget(source_widget);
  }
}

WidgetValue::WidgetValue(const TBID& name, Value::Type type)
    : m_name(name), m_value(type) {}

WidgetValue::~WidgetValue() {
  while (m_connections.GetFirst()) {
    m_connections.GetFirst()->Disconnect();
  }
}

void WidgetValue::SetFromWidget(Widget* source_widget) {
  if (m_syncing) {
    // We ended up here because syncing is in progress.
    return;
  }

  // Get the value in the format.
  switch (m_value.GetType()) {
    case Value::Type::kString:
      m_value.SetString(source_widget->GetText());
      break;
    case Value::Type::kNull:
    case Value::Type::kInt:
      m_value.SetInt(source_widget->GetValue());
      break;
    case Value::Type::kFloat:
      // FIX: Value should use double instead of float?
      m_value.SetFloat(float(source_widget->GetValueDouble()));
      break;
    default:
      assert(!"Unsupported value type!");
      break;
  }

  SyncToWidgets(source_widget);
}

void WidgetValue::SyncToWidgets(Widget* exclude_widget) {
  // FIX: Assign group to each value. Currently we only have one global group.
  g_value_group.InvokeOnValueChanged(this);

  auto iter = m_connections.IterateForward();
  while (WidgetValueConnection* connection = iter.GetAndStep()) {
    if (connection->m_widget != exclude_widget) {
      SyncToWidget(connection->m_widget);
    }
  }
}

void WidgetValue::SyncToWidget(Widget* dst_widget) {
  if (m_syncing) {
    // We ended up here because syncing is in progress.
    return;
  }

  m_syncing = true;
  switch (m_value.GetType()) {
    case Value::Type::kString:
      dst_widget->SetText(m_value.GetString());
      break;
    case Value::Type::kNull:
    case Value::Type::kInt:
      dst_widget->SetValue(m_value.GetInt());
      break;
    case Value::Type::kFloat:
      // FIX: Value should use double instead of float?
      dst_widget->SetValueDouble(m_value.GetFloat());
      break;
    default:
      assert(!"Unsupported value type!");
      break;
  }
  m_syncing = false;
}

void WidgetValue::SetInt(int value) {
  m_value.SetInt(value);
  SyncToWidgets(nullptr);
}

void WidgetValue::SetText(const char* text) {
  m_value.SetString(text, Value::Set::kNewCopy);
  SyncToWidgets(nullptr);
}

void WidgetValue::SetDouble(double value) {
  // FIX: Value should use double instead of float?
  m_value.SetFloat(float(value));
  SyncToWidgets(nullptr);
}

/*extern*/ ValueGroup g_value_group;

WidgetValue* ValueGroup::CreateValueIfNeeded(const TBID& name,
                                             Value::Type type) {
  if (WidgetValue* val = GetValue(name)) {
    return val;
  }
  WidgetValue* val = new WidgetValue(name, type);
  m_values.Add(name, val);
  return val;
}

void ValueGroup::InvokeOnValueChanged(const WidgetValue* value) {
  auto iter = m_listeners.IterateForward();
  while (ValueGroupListener* listener = iter.GetAndStep()) {
    listener->OnValueChanged(this, value);
  }
}

}  // namespace tb
