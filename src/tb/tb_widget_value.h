/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WIDGET_VALUE_H
#define TB_WIDGET_VALUE_H

#include "tb_core.h"
#include "tb_id.h"
#include "tb_linklist.h"
#include "tb_hashtable.h"
#include "tb_value.h"

namespace tb {

class ValueGroup;
class Widget;
class WidgetValue;

// Maintains a connection between Widget and WidgetValue.
class WidgetValueConnection : public TBLinkOf<WidgetValueConnection> {
 public:
  WidgetValueConnection() = default;
  ~WidgetValueConnection() { Disconnect(); }

  // Connects the value and widget.
  void Connect(WidgetValue* value, Widget* m_widget);

  // Disconnects the value and widget if it is connected.
  void Disconnect();

  // Synchronizes the value of the widget to the WidgetValue and all other
  // connected widgets.
  void SyncFromWidget(Widget* source_widget);

 private:
  friend class WidgetValue;
  WidgetValue* m_value = nullptr;
  Widget* m_widget = nullptr;
};

// Stores a Value that will be synchronized with all widgets connected to it.
// It has a TBID name, that can be used to identify this value within its
// ValueGroup.
//
// It will synchronize with widgets when any of the connected widgets change and
// triggers the EventType::kChanged event, and when the value is changed with
// any of the setters here.
//
// The synchronization with widgets is done through the generic Widget
// setters/getters,
// Widget::SetValue/GetValue/SetValueDouble/GetValueDouble/GetText/SetText.
//
// The type that is synchronized is determined by the Value::Type specified in
// the constructor.
//
// NOTE: The type that is synchronized changes if you request it in a different
// format!
class WidgetValue {
 public:
  WidgetValue(const TBID& name, Value::Type type = Value::Type::kInt);
  ~WidgetValue();

  TBID GetName() const { return m_name; }

  // Sets integer value and sync to connected widgets.
  void SetInt(int value);

  // Sets text value and sync to connected widgets.
  void SetText(const char* text);

  // Sets double value and sync to connected widgets.
  void SetDouble(double value);

  // Sets the value from the given widget. Using the current format type.
  void SetFromWidget(Widget* source_widget);

  int GetInt() { return m_value.GetInt(); }
  std::string GetText() { return m_value.GetString(); }
  double GetDouble() { return m_value.GetFloat(); }
  const Value& GetValue() const { return m_value; }

 private:
  friend class WidgetValueConnection;

  void SyncToWidget(Widget* dst_widget);
  void SyncToWidgets(Widget* exclude_widget);

  TBID m_name;
  Value m_value;
  TBLinkListOf<WidgetValueConnection> m_connections;
  bool m_syncing = false;
};

// Listener that will be notified when any of the values in a ValueGroup is
// changed.
class ValueGroupListener : public TBLinkOf<ValueGroupListener> {
 public:
  virtual ~ValueGroupListener() {
    if (linklist) {
      linklist->Remove(this);
    }
  }

  // Called when a value has changed and all widgets connected to it has been
  // updated.
  virtual void OnValueChanged(const ValueGroup* group,
                              const WidgetValue* value) = 0;
};

// ValueGroup is a collection of widget values (WidgetValue) that can be fetched
// by name (using a TBID). It also keeps a list of ValueGroupListener that
// listens to changes to any of the values.
class ValueGroup {
 public:
  // Creates a WidgetValue with the given name if it does not already exist.
  WidgetValue* CreateValueIfNeeded(const TBID& name,
                                   Value::Type type = Value::Type::kInt);

  // Gets the WidgetValue with the given name, or nullptr if no match is found.
  WidgetValue* GetValue(const TBID& name) const { return m_values.Get(name); }

  // Adds a listener to this group.
  // It will be removed automatically when deleted, but can also be removed by
  // RemoveListener.
  void AddListener(ValueGroupListener* listener) {
    m_listeners.AddLast(listener);
  }

  // Removes a listener from this group.
  void RemoveListener(ValueGroupListener* listener) {
    m_listeners.Remove(listener);
  }

 private:
  friend class WidgetValue;
  void InvokeOnValueChanged(const WidgetValue* value);

  TBHashTableAutoDeleteOf<WidgetValue> m_values;
  TBLinkListOf<ValueGroupListener> m_listeners;
};

// The global value group.
extern ValueGroup g_value_group;

}  // namespace tb

#endif  // TB_WIDGET_VALUE_H
