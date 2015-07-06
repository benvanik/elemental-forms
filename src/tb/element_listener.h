/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENT_LISTENER_H_
#define TB_ELEMENT_LISTENER_H_

#include "tb/util/intrusive_list.h"

namespace tb {

class Element;
class Event;

// Should never be created or subclassed anywhere except in ElementListener.
// It's only purpose is to add a extra typed link for ElementListener, since it
// needs to be added in multiple lists.
class ElementListenerGlobalLink
    : public util::IntrusiveListEntry<ElementListenerGlobalLink> {};

// Listens to some callbacks from Element.
// It may either listen to all elements globally, or one specific element.
// Local listeners (added with Element:AddListener) will be invoked before
// global listeners (added with ElementListener::AddGlobalListener).
class ElementListener : public util::IntrusiveListEntry<ElementListener>,
                        public ElementListenerGlobalLink {
 public:
  // Adds a listener to all elements.
  static void AddGlobalListener(ElementListener* listener);
  static void RemoveGlobalListener(ElementListener* listener);

  // Called when element is being deleted (in its destructor, so virtual
  // functions are already gone).
  virtual void OnElementDelete(Element* element) {}

  // Called when the element request to be deleted.
  // Return true if you want the element to not die immediately, f.ex. to fade
  // it out before it is deleted. If you return true, it's up to you to
  // finally remove it from its parent delete it.
  // Remember that the element may still be deleted prematurely for many other
  // reasons (f.ex if its parent is deleted or several listeners respond true
  // and take on the task to delete it at some point). You can use
  // WeakElementPointer to safely handle that.
  virtual bool OnElementDying(Element* element) { return false; }

  // Called when the child has been added to parent, after its parents
  // OnChildAdded.
  // Local listeners are invoked on the parent element.
  virtual void OnElementAdded(Element* parent, Element* child) {}

  // Called when the child is about to be removed from parent, after its parents
  // OnChildRemove.
  // Local listeners are invoked on the parent element.
  virtual void OnElementRemove(Element* parent, Element* child) {}

  // Called when element focus has changed on a element.
  virtual void OnElementFocusChanged(Element* element, bool focused) {}

  // Called when a event is about to be invoked on a element. This make it
  // possible to intercept events before they are handled, and block it (by
  // returning true).
  // Note, if returning true, other global listeners will still also be
  // notified.
  virtual bool OnElementInvokeEvent(Element* element, const Event& ev) {
    return false;
  }

 private:
  friend class Element;
  static void InvokeElementDelete(Element* element);
  static bool InvokeElementDying(Element* element);
  static void InvokeElementAdded(Element* parent, Element* child);
  static void InvokeElementRemove(Element* parent, Element* child);
  static void InvokeElementFocusChanged(Element* element, bool focused);
  static bool InvokeElementInvokeEvent(Element* element, const Event& ev);
};

// Keeps a pointer to a element that will be set to nullptr if the element is
// removed.
class WeakElementPointer : private ElementListener {
 public:
  WeakElementPointer() = default;
  WeakElementPointer(Element* element) { reset(element); }
  ~WeakElementPointer() { reset(nullptr); }

  // Sets the element pointer that should be nulled if deleted.
  void reset(Element* element = nullptr);

  // Returns the element, or nullptr if it has been deleted.
  Element* get() const { return m_element; }

  operator bool() { return m_element != nullptr; }

 private:
  void OnElementDelete(Element* element) override;

  Element* m_element = nullptr;
};

}  // namespace tb

#endif  // TB_ELEMENT_LISTENER_H_
