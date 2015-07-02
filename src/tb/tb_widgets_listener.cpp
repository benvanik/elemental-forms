/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widgets_listener.h"

namespace tb {

TBLinkListOf<WidgetListenerGlobalLink> g_listeners;

void WidgetListener::AddGlobalListener(WidgetListener* listener) {
  g_listeners.AddLast(listener);
}

void WidgetListener::RemoveGlobalListener(WidgetListener* listener) {
  g_listeners.Remove(listener);
}

void WidgetListener::InvokeWidgetDelete(TBWidget* widget) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = widget->m_listeners.IterateForward();
  while (WidgetListener* listener = local_i.GetAndStep()) {
    listener->OnWidgetDelete(widget);
  }
  while (WidgetListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<WidgetListener*>(link)->OnWidgetDelete(widget);
  }
}

bool WidgetListener::InvokeWidgetDying(TBWidget* widget) {
  bool handled = false;
  auto global_i = g_listeners.IterateForward();
  auto local_i = widget->m_listeners.IterateForward();
  while (WidgetListener* listener = local_i.GetAndStep()) {
    handled |= listener->OnWidgetDying(widget);
  }
  while (WidgetListenerGlobalLink* link = global_i.GetAndStep()) {
    handled |= static_cast<WidgetListener*>(link)->OnWidgetDying(widget);
  }
  return handled;
}

void WidgetListener::InvokeWidgetAdded(TBWidget* parent, TBWidget* child) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = parent->m_listeners.IterateForward();
  while (WidgetListener* listener = local_i.GetAndStep()) {
    listener->OnWidgetAdded(parent, child);
  }
  while (WidgetListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<WidgetListener*>(link)->OnWidgetAdded(parent, child);
  }
}

void WidgetListener::InvokeWidgetRemove(TBWidget* parent, TBWidget* child) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = parent->m_listeners.IterateForward();
  while (WidgetListener* listener = local_i.GetAndStep()) {
    listener->OnWidgetRemove(parent, child);
  }
  while (WidgetListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<WidgetListener*>(link)->OnWidgetRemove(parent, child);
  }
}

void WidgetListener::InvokeWidgetFocusChanged(TBWidget* widget, bool focused) {
  auto global_i = g_listeners.IterateForward();
  auto local_i = widget->m_listeners.IterateForward();
  while (WidgetListener* listener = local_i.GetAndStep()) {
    listener->OnWidgetFocusChanged(widget, focused);
  }
  while (WidgetListenerGlobalLink* link = global_i.GetAndStep()) {
    static_cast<WidgetListener*>(link)->OnWidgetFocusChanged(widget, focused);
  }
}

bool WidgetListener::InvokeWidgetInvokeEvent(TBWidget* widget,
                                             const TBWidgetEvent& ev) {
  bool handled = false;
  auto global_i = g_listeners.IterateForward();
  auto local_i = widget->m_listeners.IterateForward();
  while (WidgetListener* listener = local_i.GetAndStep()) {
    handled |= listener->OnWidgetInvokeEvent(widget, ev);
  }
  while (WidgetListenerGlobalLink* link = global_i.GetAndStep()) {
    handled |=
        static_cast<WidgetListener*>(link)->OnWidgetInvokeEvent(widget, ev);
  }
  return handled;
}

void WeakWidgetPointer::Set(TBWidget* widget) {
  if (m_widget == widget) {
    return;
  }
  if (m_widget) {
    m_widget->RemoveListener(this);
  }
  m_widget = widget;
  if (m_widget) {
    m_widget->AddListener(this);
  }
}

void WeakWidgetPointer::OnWidgetDelete(TBWidget* widget) {
  if (widget == m_widget) {
    Set(nullptr);
  }
}

}  // namespace tb
