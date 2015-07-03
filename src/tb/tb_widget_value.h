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

class Widget;
class WidgetValue;
class TBValueGroup;

/** WidgetValueConnection maintains a connection between Widget and
 * WidgetValue. */

class WidgetValueConnection : public TBLinkOf<WidgetValueConnection> {
 public:
  WidgetValueConnection() : m_value(nullptr) {}
  ~WidgetValueConnection() { Unconnect(); }

  /** Connect the value and widget. */
  void Connect(WidgetValue* value, Widget* m_widget);

  /** Unconnect the value and widget if it is connected. */
  void Unconnect();

  /** Synchronize the value of the widget to the WidgetValue and all other
          connected widgets. */
  void SyncFromWidget(Widget* source_widget);

 private:
  friend class WidgetValue;
  WidgetValue* m_value;
  Widget* m_widget;
};

/** WidgetValue stores a TBValue that will be synchronized with all widgets
   connected to it.
        It has a TBID name, that can be used to identify this value within its
   TBValueGroup.

        It will synchronize with widgets when any of the connected widgets
   change and trig the
        EventType::kChanged event, and when the value is changed with any of the
   setters here.

        The synchronization with widgets is done through the generic Widget
   setters/getters,
        Widget::SetValue/GetValue/SetValueDouble/GetValueDouble/GetText/SetText.

        The type that is synchronized is determined by the TBValue::Type
   specified in the
        constructor.

        Note: The type that is synchronized changes if you request it in a
   different format!
*/

class WidgetValue {
 public:
  WidgetValue(const TBID& name, TBValue::Type type = TBValue::Type::kInt);
  ~WidgetValue();

  /** Set integer value and sync to connected widgets. */
  void SetInt(int value);

  /** Set text value and sync to connected widgets. */
  void SetText(const char* text);

  /** Set double value and sync to connected widgets. */
  void SetDouble(double value);

  /** Set the value from the given widget. Using the current format type.*/
  void SetFromWidget(Widget* source_widget);

  /** Get value as integer. */
  int GetInt() { return m_value.GetInt(); }

  /** Get value as text. */
  std::string GetText() { return m_value.GetString(); }

  /** Get the value as double. */
  double GetDouble() { return m_value.GetFloat(); }

  /** Get the TBValue used to store the value. */
  const TBValue& GetValue() const { return m_value; }

  /** Get the name id. */
  TBID GetName() const { return m_name; }

 private:
  friend class WidgetValueConnection;
  TBID m_name;
  TBValue m_value;
  TBLinkListOf<WidgetValueConnection> m_connections;
  bool m_syncing;

  void SyncToWidget(Widget* dst_widget);
  void SyncToWidgets(Widget* exclude_widget);
};

/** Listener that will be notified when any of the values in a TBValueGroup is
 * changed. */

class TBValueGroupListener : public TBLinkOf<TBValueGroupListener> {
 public:
  virtual ~TBValueGroupListener() {
    if (linklist) linklist->Remove(this);
  }

  /** Called when a value has changed and all widgets connected to it has been
   * updated. */
  virtual void OnValueChanged(const TBValueGroup* group,
                              const WidgetValue* value) = 0;
};

/** TBValueGroup is a collection of widget values (WidgetValue) that can be
   fetched
        by name (using a TBID). It also keeps a list of TBValueGroupListener
   that listens to
        changes to any of the values. */

class TBValueGroup {
 public:
  /** Create a WidgetValue with the given name if it does not already exist.
          Returns nullptr if out of memory. */
  WidgetValue* CreateValueIfNeeded(const TBID& name,
                                   TBValue::Type type = TBValue::Type::kInt);

  /** Get the WidgetValue with the given name, or nullptr if no match is
   * found. */
  WidgetValue* GetValue(const TBID& name) const { return m_values.Get(name); }

  /** Add listener to this group. It will be removed automatically when deleted,
          but can also be removed by RemoveListener. */
  void AddListener(TBValueGroupListener* listener) {
    m_listeners.AddLast(listener);
  }

  /** Remove listener from this group. */
  void RemoveListener(TBValueGroupListener* listener) {
    m_listeners.Remove(listener);
  }

 private:
  friend class WidgetValue;
  void InvokeOnValueChanged(const WidgetValue* value);

  TBHashTableAutoDeleteOf<WidgetValue> m_values;   ///< Hash table of values
  TBLinkListOf<TBValueGroupListener> m_listeners;  ///< List of listeners
};

/** The global value group. */
extern TBValueGroup g_value_group;

}  // namespace tb

#endif  // TB_WIDGET_VALUE_H
