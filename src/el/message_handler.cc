/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstddef>

#include "el/message_handler.h"
#include "el/util/metrics.h"
#include "el/util/timer.h"

namespace el {

// List of all delayed messages.
util::IntrusiveList<MessageLink> g_all_delayed_messages;
// List of all nondelayed messages.
util::IntrusiveList<MessageLink> g_all_normal_messages;

MessageHandler::MessageHandler() = default;

MessageHandler::~MessageHandler() { DeleteAllMessages(); }

void MessageHandler::PostMessageDelayed(TBID message_id,
                                        std::unique_ptr<MessageData> data,
                                        uint32_t delay_in_ms) {
  PostMessageOnTime(message_id, std::move(data),
                    util::GetTimeMS() + delay_in_ms);
}

void MessageHandler::PostMessageOnTime(TBID message_id,
                                       std::unique_ptr<MessageData> data,
                                       uint64_t fire_time) {
  Message* msg = new Message(message_id, std::move(data), fire_time, this);
  // Find the message that is already in the list that should fire later, so we
  // can insert msg just before that. (Always keep the list ordered after fire
  // time).

  // NOTE: If another message is added during OnMessageReceived, it might or
  // might not be fired in the right order compared to other delayed messages,
  // depending on if it's inserted before or after the message being processed!

  Message* later_msg = nullptr;
  MessageLink* link = g_all_delayed_messages.GetFirst();
  while (link) {
    Message* msg_in_list = static_cast<Message*>(link);
    if (msg_in_list->fire_time_millis() > msg->fire_time_millis()) {
      later_msg = msg_in_list;
      break;
    }
    link = link->GetNext();
  }

  // Add it to the global list in the right order.
  if (later_msg) {
    g_all_delayed_messages.AddBefore(msg, later_msg);
  } else {
    g_all_delayed_messages.AddLast(msg);
  }

  // Add it to the list in messagehandler.
  m_messages.AddLast(msg);

  // If we added it first and there's no normal messages, the next fire time has
  // changed and we have to reschedule the timer.
  if (!g_all_normal_messages.GetFirst() &&
      g_all_delayed_messages.GetFirst() == msg) {
    util::RescheduleTimer(msg->fire_time_millis());
  }
}

void MessageHandler::PostMessage(TBID message_id,
                                 std::unique_ptr<MessageData> data) {
  Message* msg = new Message(message_id, std::move(data), 0, this);
  g_all_normal_messages.AddLast(msg);
  m_messages.AddLast(msg);

  // If we added it and there was no messages, the next fire time has
  // changed and we have to rescedule the timer.
  if (g_all_normal_messages.GetFirst() == msg) {
    util::RescheduleTimer(0);
  }
}

Message* MessageHandler::GetMessageById(TBID message_id) {
  auto iter = m_messages.IterateForward();
  while (Message* msg = iter.GetAndStep()) {
    if (msg->message_id() == message_id) {
      return msg;
    }
  }
  return nullptr;
}

void MessageHandler::DeleteMessage(Message* msg) {
  // Ensure the same message handler.
  assert(msg->message_handler() == this);

  // Remove from global list (g_all_delayed_messages or g_all_normal_messages)
  if (g_all_delayed_messages.ContainsLink(msg)) {
    g_all_delayed_messages.Remove(msg);
  } else if (g_all_normal_messages.ContainsLink(msg)) {
    g_all_normal_messages.Remove(msg);
  }

  // Remove from local list
  m_messages.Remove(msg);

  delete msg;

  // Note: We could call util::RescheduleTimer if we think that deleting
  // this message changed the time for the next message.
}

void MessageHandler::DeleteAllMessages() {
  while (Message* msg = m_messages.GetFirst()) {
    DeleteMessage(msg);
  }
}

// static
void MessageHandler::ProcessMessages() {
  // Handle delayed messages.
  auto iter = g_all_delayed_messages.IterateForward();
  while (Message* msg = static_cast<Message*>(iter.GetAndStep())) {
    if (util::GetTimeMS() >= msg->fire_time_millis()) {
      // Remove from global list.
      g_all_delayed_messages.Remove(msg);
      // Remove from local list.
      msg->message_handler()->m_messages.Remove(msg);

      msg->message_handler()->OnMessageReceived(msg);

      delete msg;
    } else {
      // Since the list is sorted, all remaining messages should fire later.
      break;
    }
  }

  // Handle normal messages.
  iter = g_all_normal_messages.IterateForward();
  while (Message* msg = static_cast<Message*>(iter.GetAndStep())) {
    // Remove from global list.
    g_all_normal_messages.Remove(msg);
    // Remove from local list.
    msg->message_handler()->m_messages.Remove(msg);

    msg->message_handler()->OnMessageReceived(msg);

    delete msg;
  }
}

// static
uint64_t MessageHandler::GetNextMessageFireTime() {
  if (g_all_normal_messages.GetFirst()) {
    return 0;
  }

  if (g_all_delayed_messages.GetFirst()) {
    auto first_delayed_msg =
        static_cast<Message*>(g_all_delayed_messages.GetFirst());
    return first_delayed_msg->fire_time_millis();
  }

  return kNotSoon;
}

}  // namespace el
