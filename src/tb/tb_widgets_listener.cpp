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

TBLinkListOf<TBWidgetListenerGlobalLink> g_listeners;

void TBWidgetListener::AddGlobalListener(TBWidgetListener* listener) {
  g_listeners.AddLast(listener);
}

void TBWidgetListener::RemoveGlobalListener(TBWidgetListener* listener) {
  g_listeners.Remove(listener);
}

void TBWidgetListener::InvokeWidgetDelete(TBWidget* widget) {
  TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i =
      g_listeners.IterateForward();
  TBLinkListOf<TBWidgetListener>::Iterator local_i =
      widget->m_listeners.IterateForward();
  while (TBWidgetListener* listener = local_i.GetAndStep())
    listener->OnWidgetDelete(widget);
  while (TBWidgetListenerGlobalLink* link = global_i.GetAndStep())
    static_cast<TBWidgetListener*>(link)->OnWidgetDelete(widget);
}

bool TBWidgetListener::InvokeWidgetDying(TBWidget* widget) {
  bool handled = false;
  TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i =
      g_listeners.IterateForward();
  TBLinkListOf<TBWidgetListener>::Iterator local_i =
      widget->m_listeners.IterateForward();
  while (TBWidgetListener* listener = local_i.GetAndStep())
    handled |= listener->OnWidgetDying(widget);
  while (TBWidgetListenerGlobalLink* link = global_i.GetAndStep())
    handled |= static_cast<TBWidgetListener*>(link)->OnWidgetDying(widget);
  return handled;
}

void TBWidgetListener::InvokeWidgetAdded(TBWidget* parent, TBWidget* child) {
  TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i =
      g_listeners.IterateForward();
  TBLinkListOf<TBWidgetListener>::Iterator local_i =
      parent->m_listeners.IterateForward();
  while (TBWidgetListener* listener = local_i.GetAndStep())
    listener->OnWidgetAdded(parent, child);
  while (TBWidgetListenerGlobalLink* link = global_i.GetAndStep())
    static_cast<TBWidgetListener*>(link)->OnWidgetAdded(parent, child);
}

void TBWidgetListener::InvokeWidgetRemove(TBWidget* parent, TBWidget* child) {
  TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i =
      g_listeners.IterateForward();
  TBLinkListOf<TBWidgetListener>::Iterator local_i =
      parent->m_listeners.IterateForward();
  while (TBWidgetListener* listener = local_i.GetAndStep())
    listener->OnWidgetRemove(parent, child);
  while (TBWidgetListenerGlobalLink* link = global_i.GetAndStep())
    static_cast<TBWidgetListener*>(link)->OnWidgetRemove(parent, child);
}

void TBWidgetListener::InvokeWidgetFocusChanged(TBWidget* widget,
                                                bool focused) {
  TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i =
      g_listeners.IterateForward();
  TBLinkListOf<TBWidgetListener>::Iterator local_i =
      widget->m_listeners.IterateForward();
  while (TBWidgetListener* listener = local_i.GetAndStep())
    listener->OnWidgetFocusChanged(widget, focused);
  while (TBWidgetListenerGlobalLink* link = global_i.GetAndStep())
    static_cast<TBWidgetListener*>(link)->OnWidgetFocusChanged(widget, focused);
}

bool TBWidgetListener::InvokeWidgetInvokeEvent(TBWidget* widget,
                                               const TBWidgetEvent& ev) {
  bool handled = false;
  TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i =
      g_listeners.IterateForward();
  TBLinkListOf<TBWidgetListener>::Iterator local_i =
      widget->m_listeners.IterateForward();
  while (TBWidgetListener* listener = local_i.GetAndStep())
    handled |= listener->OnWidgetInvokeEvent(widget, ev);
  while (TBWidgetListenerGlobalLink* link = global_i.GetAndStep())
    handled |=
        static_cast<TBWidgetListener*>(link)->OnWidgetInvokeEvent(widget, ev);
  return handled;
}

void TBWidgetSafePointer::Set(TBWidget* widget) {
  if (m_widget == widget) return;
  if (m_widget) m_widget->RemoveListener(this);
  m_widget = widget;
  if (m_widget) m_widget->AddListener(this);
}

void TBWidgetSafePointer::OnWidgetDelete(TBWidget* widget) {
  if (widget == m_widget) Set(nullptr);
}

}  // namespace tb
