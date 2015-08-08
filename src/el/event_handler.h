/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_EVENT_HANDLER_H_
#define EL_EVENT_HANDLER_H_

#include <functional>
#include <list>
#include <memory>

#include "el/element_listener.h"
#include "el/event.h"
#include "el/util/intrusive_list.h"

namespace el {

class EventHandler : public util::IntrusiveListEntry<EventHandler> {
 public:
  explicit EventHandler(Element* scope_element);
  ~EventHandler();

  void Listen(EventType event_type,
              std::function<bool(const Event& ev)> callback);
  void Listen(EventType event_type, const TBID& target_element_id,
              std::function<bool(const Event& ev)> callback);
  void Listen(EventType event_type, Element* target_element,
              std::function<bool(const Event& ev)> callback);

  bool OnEvent(const Event& ev);

 private:
  struct Listener;

  WeakElementPointer scope_element_;
  std::list<std::unique_ptr<Listener>> listeners_;
};

}  // namespace el

#endif  // EL_EVENT_HANDLER_H_
