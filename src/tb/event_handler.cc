/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/element.h"
#include "tb/event_handler.h"

namespace tb {

struct EventHandler::Listener {
  enum class Filter {
    kAll,
    kTargetElementId,
    kTargetElement,
  };
  EventType event_type;
  Filter filter;
  TBID target_element_id;
  Element* target_element = nullptr;
  std::function<bool(const Event& ev)> callback;
  bool Matches(const Event& ev) {
    return ev.type == event_type &&
           (filter == Filter::kAll ||
            (filter == Filter::kTargetElement && ev.target == target_element) ||
            (filter == Filter::kTargetElementId &&
             ev.target->id() == target_element_id));
  }
};

EventHandler::EventHandler(Element* scope_element)
    : scope_element_(scope_element) {
  scope_element->AddEventHandler(this);
}

EventHandler::~EventHandler() {
  if (scope_element_.get()) {
    scope_element_.get()->RemoveEventHandler(this);
  }
}

void EventHandler::Listen(EventType event_type,
                          std::function<bool(const Event& ev)> callback) {
  auto listener = std::make_unique<Listener>();
  listener->event_type = event_type;
  listener->filter = Listener::Filter::kAll;
  listener->callback = std::move(callback);
  listeners_.emplace_back(std::move(listener));
}

void EventHandler::Listen(EventType event_type, const TBID& target_element_id,
                          std::function<bool(const Event& ev)> callback) {
  auto listener = std::make_unique<Listener>();
  listener->event_type = event_type;
  listener->filter = Listener::Filter::kTargetElementId;
  listener->target_element_id = target_element_id;
  listener->callback = std::move(callback);
  listeners_.emplace_back(std::move(listener));
}

void EventHandler::Listen(EventType event_type, Element* target_element,
                          std::function<bool(const Event& ev)> callback) {
  auto listener = std::make_unique<Listener>();
  listener->event_type = event_type;
  listener->filter = Listener::Filter::kTargetElement;
  listener->target_element = target_element;
  listener->callback = std::move(callback);
  listeners_.emplace_back(std::move(listener));
}

bool EventHandler::OnEvent(const Event& ev) {
  for (auto& it : listeners_) {
    if (it->Matches(ev)) {
      if (it->callback(ev)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace tb
