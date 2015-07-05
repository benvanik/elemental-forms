/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/element.h"
#include "tb/element_listener.h"

namespace tb {

util::TBLinkListOf<ElementListenerGlobalLink> g_listeners;

void ElementListener::AddGlobalListener(ElementListener* listener) {
  g_listeners.AddLast(listener);
}

void ElementListener::RemoveGlobalListener(ElementListener* listener) {
  g_listeners.Remove(listener);
}

void ElementListener::InvokeElementDelete(Element* element) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = element->m_listeners.IterateForward();
  while (ElementListener* listener = local_i.GetAndStep()) {
    listener->OnElementDelete(element);
  }
  while (ElementListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<ElementListener*>(link)->OnElementDelete(element);
  }
}

bool ElementListener::InvokeElementDying(Element* element) {
  bool handled = false;
  auto global_i = g_listeners.IterateForward();
  auto local_i = element->m_listeners.IterateForward();
  while (ElementListener* listener = local_i.GetAndStep()) {
    handled |= listener->OnElementDying(element);
  }
  while (ElementListenerGlobalLink* link = global_i.GetAndStep()) {
    handled |= static_cast<ElementListener*>(link)->OnElementDying(element);
  }
  return handled;
}

void ElementListener::InvokeElementAdded(Element* parent, Element* child) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = parent->m_listeners.IterateForward();
  while (ElementListener* listener = local_i.GetAndStep()) {
    listener->OnElementAdded(parent, child);
  }
  while (ElementListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<ElementListener*>(link)->OnElementAdded(parent, child);
  }
}

void ElementListener::InvokeElementRemove(Element* parent, Element* child) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = parent->m_listeners.IterateForward();
  while (ElementListener* listener = local_i.GetAndStep()) {
    listener->OnElementRemove(parent, child);
  }
  while (ElementListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<ElementListener*>(link)->OnElementRemove(parent, child);
  }
}

void ElementListener::InvokeElementFocusChanged(Element* element,
                                                bool focused) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = element->m_listeners.IterateForward();
  while (ElementListener* listener = local_i.GetAndStep()) {
    listener->OnElementFocusChanged(element, focused);
  }
  while (ElementListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<ElementListener*>(link)
        ->OnElementFocusChanged(element, focused);
  }
}

bool ElementListener::InvokeElementInvokeEvent(Element* element,
                                               const ElementEvent& ev) {
  bool handled = false;
  auto global_i = g_listeners.IterateForward();
  auto local_i = element->m_listeners.IterateForward();
  while (ElementListener* listener = local_i.GetAndStep()) {
    handled |= listener->OnElementInvokeEvent(element, ev);
  }
  while (ElementListenerGlobalLink* link = global_i.GetAndStep()) {
    handled |=
        static_cast<ElementListener*>(link)->OnElementInvokeEvent(element, ev);
  }
  return handled;
}

void WeakElementPointer::Set(Element* element) {
  if (m_element == element) {
    return;
  }
  if (m_element) {
    m_element->RemoveListener(this);
  }
  m_element = element;
  if (m_element) {
    m_element->AddListener(this);
  }
}

void WeakElementPointer::OnElementDelete(Element* element) {
  if (element == m_element) {
    Set(nullptr);
  }
}

}  // namespace tb
