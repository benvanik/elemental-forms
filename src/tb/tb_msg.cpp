/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_msg.h"

#include <cstddef>

#include "tb/util/metrics.h"
#include "tb/util/timer.h"

namespace tb {

// List of all delayed messages.
util::TBLinkListOf<MessageLink> g_all_delayed_messages;
// List of all nondelayed messages.
util::TBLinkListOf<MessageLink> g_all_normal_messages;

Message::Message(TBID message, MessageData* data, uint64_t fire_time_ms,
                 MessageHandler* mh)
    : message(message), data(data), fire_time_ms(fire_time_ms), mh(mh) {}

Message::~Message() { delete data; }

MessageHandler::MessageHandler() = default;

MessageHandler::~MessageHandler() { DeleteAllMessages(); }

void MessageHandler::PostMessageDelayed(TBID message, MessageData* data,
                                        uint32_t delay_in_ms) {
  PostMessageOnTime(message, data, util::GetTimeMS() + delay_in_ms);
}

void MessageHandler::PostMessageOnTime(TBID message, MessageData* data,
                                       uint64_t fire_time) {
  Message* msg = new Message(message, data, fire_time, this);
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
    if (msg_in_list->fire_time_ms > msg->fire_time_ms) {
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
    util::RescheduleTimer(msg->fire_time_ms);
  }
}

void MessageHandler::PostMessage(TBID message, MessageData* data) {
  Message* msg = new Message(message, data, 0, this);
  g_all_normal_messages.AddLast(msg);
  m_messages.AddLast(msg);

  // If we added it and there was no messages, the next fire time has
  // changed and we have to rescedule the timer.
  if (g_all_normal_messages.GetFirst() == msg) {
    util::RescheduleTimer(0);
  }
}

Message* MessageHandler::GetMessageByID(TBID message) {
  auto iter = m_messages.IterateForward();
  while (Message* msg = iter.GetAndStep()) {
    if (msg->message == message) {
      return msg;
    }
  }
  return nullptr;
}

void MessageHandler::DeleteMessage(Message* msg) {
  // Ensure the same message handler.
  assert(msg->mh == this);

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
    if (util::GetTimeMS() >= msg->fire_time_ms) {
      // Remove from global list.
      g_all_delayed_messages.Remove(msg);
      // Remove from local list.
      msg->mh->m_messages.Remove(msg);

      msg->mh->OnMessageReceived(msg);

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
    msg->mh->m_messages.Remove(msg);

    msg->mh->OnMessageReceived(msg);

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
    return first_delayed_msg->fire_time_ms;
  }

  return kNotSoon;
}

}  // namespace tb
