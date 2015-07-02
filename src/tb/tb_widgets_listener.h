/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WIDGETSLISTENER_H
#define TB_WIDGETSLISTENER_H

#include "tb_core.h"
#include "tb_linklist.h"
#include "tb_widgets.h"

namespace tb {

class TBWidget;

// Should never be created or subclassed anywhere except in WidgetListener. It's
// only purpose is to add a extra typed link for WidgetListener, since it needs
// to be added in multiple lists.
class WidgetListenerGlobalLink : public TBLinkOf<WidgetListenerGlobalLink> {};

// Listens to some callbacks from TBWidget.
// It may either listen to all widgets globally, or one specific widget.
// Local listeners (added with TBWidget:AddListener) will be invoked before
// global listeners (added with WidgetListener::AddGlobalListener).
class WidgetListener : public TBLinkOf<WidgetListener>,
                       public WidgetListenerGlobalLink {
 public:
  // Adds a listener to all widgets.
  static void AddGlobalListener(WidgetListener* listener);
  static void RemoveGlobalListener(WidgetListener* listener);

  // Called when widget is being deleted (in its destructor, so virtual
  // functions are already gone).
  virtual void OnWidgetDelete(TBWidget* widget) {}

  // Called when the widget request to be deleted.
  // Return true if you want the widget to not die immediately, f.ex. to fade
  // it out before it is deleted. If you return true, it's up to you to
  // finally remove it from its parent delete it.
  // Remember that the widget may still be deleted prematurely for many other
  // reasons (f.ex if its parent is deleted or several listeners respond true
  // and take on the task to delete it at some point). You can use
  // WeakWidgetPointer to safely handle that.
  virtual bool OnWidgetDying(TBWidget* widget) { return false; }

  // Called when the child has been added to parent, after its parents
  // OnChildAdded.
  // Local listeners are invoked on the parent widget.
  virtual void OnWidgetAdded(TBWidget* parent, TBWidget* child) {}

  // Called when the child is about to be removed from parent, after its parents
  // OnChildRemove.
  // Local listeners are invoked on the parent widget.
  virtual void OnWidgetRemove(TBWidget* parent, TBWidget* child) {}

  // Called when widget focus has changed on a widget.
  virtual void OnWidgetFocusChanged(TBWidget* widget, bool focused) {}

  // Called when a event is about to be invoked on a widget. This make it
  // possible to intercept events before they are handled, and block it (by
  // returning true).
  // Note, if returning true, other global listeners will still also be
  // notified.
  virtual bool OnWidgetInvokeEvent(TBWidget* widget, const TBWidgetEvent& ev) {
    return false;
  }

 private:
  friend class TBWidget;
  static void InvokeWidgetDelete(TBWidget* widget);
  static bool InvokeWidgetDying(TBWidget* widget);
  static void InvokeWidgetAdded(TBWidget* parent, TBWidget* child);
  static void InvokeWidgetRemove(TBWidget* parent, TBWidget* child);
  static void InvokeWidgetFocusChanged(TBWidget* widget, bool focused);
  static bool InvokeWidgetInvokeEvent(TBWidget* widget,
                                      const TBWidgetEvent& ev);
};

// Keeps a pointer to a widget that will be set to nullptr if the widget is
// removed.
class WeakWidgetPointer : private WidgetListener {
 public:
  WeakWidgetPointer() = default;
  WeakWidgetPointer(TBWidget* widget) { Set(widget); }
  ~WeakWidgetPointer() { Set(nullptr); }

  // Sets the widget pointer that should be nulled if deleted.
  void Set(TBWidget* widget);

  // Returns the widget, or nullptr if it has been deleted.
  TBWidget* Get() const { return m_widget; }

 private:
  void OnWidgetDelete(TBWidget* widget) override;

  TBWidget* m_widget = nullptr;
};

}  // namespace tb

#endif  // TB_WIDGETSLISTENER_H
