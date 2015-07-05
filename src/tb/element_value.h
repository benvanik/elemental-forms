/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENT_VALUE_H_
#define TB_ELEMENT_VALUE_H_

#include <memory>
#include <unordered_map>

#include "tb/id.h"
#include "tb/util/link_list.h"
#include "tb/value.h"

namespace tb {

class Element;
class ElementValue;
class ElementValueGroup;

// Maintains a connection between Element and ElementValue.
class ElementValueConnection : public util::TBLinkOf<ElementValueConnection> {
 public:
  ElementValueConnection() = default;
  ~ElementValueConnection() { Disconnect(); }

  // Connects the value and element.
  void Connect(ElementValue* value, Element* m_element);

  // Disconnects the value and element if it is connected.
  void Disconnect();

  // Synchronizes the value of the element to the ElementValue and all other
  // connected elements.
  void SyncFromElement(Element* source_element);

 private:
  friend class ElementValue;
  ElementValue* m_value = nullptr;
  Element* m_element = nullptr;
};

// Stores a Value that will be synchronized with all elements connected to it.
// It has a TBID name, that can be used to identify this value within its
// ElementValueGroup.
//
// It will synchronize with elements when any of the connected elements change
// and triggers the EventType::kChanged event, and when the value is changed
// with any of the setters here.
//
// The synchronization with elements is done through the generic Element
// setters/getters,
// Element::SetValue/GetValue/SetValueDouble/GetValueDouble/GetText/SetText.
//
// The type that is synchronized is determined by the Value::Type specified in
// the constructor.
//
// NOTE: The type that is synchronized changes if you request it in a different
// format!
class ElementValue {
 public:
  ElementValue(const TBID& name, Value::Type type = Value::Type::kInt);
  ~ElementValue();

  TBID GetName() const { return m_name; }

  // Sets integer value and sync to connected elements.
  void set_integer(int value);

  // Sets text value and sync to connected elements.
  void SetText(const char* text);

  // Sets double value and sync to connected elements.
  void SetDouble(double value);

  // Sets the value from the given element. Using the current format type.
  void SetFromElement(Element* source_element);

  int as_integer() { return m_value.as_integer(); }
  std::string GetText() { return m_value.as_string(); }
  double GetDouble() { return m_value.as_float(); }
  const Value& GetValue() const { return m_value; }

 private:
  friend class ElementValueConnection;

  void SyncToElement(Element* dst_element);
  void SyncToElements(Element* exclude_element);

  TBID m_name;
  Value m_value;
  util::TBLinkListOf<ElementValueConnection> m_connections;
  bool m_syncing = false;
};

// Listener that will be notified when any of the values in a ElementValueGroup
// is
// changed.
class ElementValueGroupListener
    : public util::TBLinkOf<ElementValueGroupListener> {
 public:
  virtual ~ElementValueGroupListener() {
    if (linklist) {
      linklist->Remove(this);
    }
  }

  // Called when a value has changed and all elements connected to it has been
  // updated.
  virtual void OnValueChanged(const ElementValueGroup* group,
                              const ElementValue* value) = 0;
};

// ElementValueGroup is a collection of element values (ElementValue) that can
// be fetched by name (using a TBID). It also keeps a list of
// ElementValueGroupListener that listens to changes to any of the values.
class ElementValueGroup {
 public:
  static ElementValueGroup* get() { return &value_group_singleton_; }

  // Creates a ElementValue with the given name if it does not already exist.
  ElementValue* CreateValueIfNeeded(const TBID& name,
                                    Value::Type type = Value::Type::kInt);

  // Gets the ElementValue with the given name, or nullptr if no match is found.
  ElementValue* GetValue(const TBID& name) const;

  // Adds a listener to this group.
  // It will be removed automatically when deleted, but can also be removed by
  // RemoveListener.
  void AddListener(ElementValueGroupListener* listener) {
    m_listeners.AddLast(listener);
  }

  // Removes a listener from this group.
  void RemoveListener(ElementValueGroupListener* listener) {
    m_listeners.Remove(listener);
  }

 private:
  friend class ElementValue;
  void InvokeOnValueChanged(const ElementValue* value);

  static ElementValueGroup value_group_singleton_;

  std::unordered_map<uint32_t, std::unique_ptr<ElementValue>> m_values;
  util::TBLinkListOf<ElementValueGroupListener> m_listeners;
};

}  // namespace tb

#endif  // TB_ELEMENT_VALUE_H_
