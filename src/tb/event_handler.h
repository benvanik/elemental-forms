/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_EVENT_HANDLER_H_
#define TB_EVENT_HANDLER_H_

#include <functional>
#include <list>
#include <memory>

#include "tb/element_listener.h"
#include "tb/event.h"
#include "tb/util/intrusive_list.h"

namespace tb {

class EventHandler : public util::IntrusiveListEntry<EventHandler> {
 public:
  EventHandler(Element* scope_element);
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

}  // namespace tb

#endif  // TB_EVENT_HANDLER_H_
