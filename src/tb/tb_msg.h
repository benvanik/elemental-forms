/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_MSG_H
#define TB_MSG_H

#include "tb_id.h"
#include "tb_value.h"

#include "tb/util/link_list.h"
#include "tb/util/object.h"

namespace tb {

class MessageHandler;

// kNotSoon is returned from MessageHandler::GetNextMessageFireTime
// and means that there is currently no more messages to process.
constexpr uint64_t kNotSoon = -1;

// Holds custom data to send with a posted message.
class MessageData : public util::TypedObject {
 public:
  MessageData() = default;
  MessageData(int v1, int v2) : v1(v1), v2(v2) {}
  virtual ~MessageData() = default;

 public:
  // Values can be used for anything.
  Value v1;
  Value v2;
  TBID id1;
  TBID id2;
};

// Should never be created or subclassed anywhere except in Message.
// It's only purpose is to add a extra typed link for Message, since it needs
// to be added in multiple lists.
class MessageLink : public util::TBLinkOf<MessageLink> {};

// A message created and owned by MessageHandler.
// It carries a message id, and may also carry a MessageData with additional
// parameters.
class Message : public util::TBLinkOf<Message>, public MessageLink {
 private:
  Message(TBID message, MessageData* data, uint64_t fire_time_ms,
          MessageHandler* mh);
  ~Message();

 public:
  TBID message;       // The message id.
  MessageData* data;  // The message data, or nullptr if no data is set.

  // Gets the time which a delayed message should have fired (0 for non delayed
  // messages).
  uint64_t GetFireTime() const { return fire_time_ms; }

 private:
  friend class MessageHandler;
  uint64_t fire_time_ms;
  MessageHandler* mh;
};

// Handles a list of pending messages posted to itself.
// Messages can be delivered immediately or after a delay.
// Delayed message are delivered as close as possible to the time they should
// fire.
// Immediate messages are put on a queue and delivered as soon as possible,
// after any delayed messages that has passed their delivery time. This queue is
// global (among all MessageHandlers).
class MessageHandler {
 public:
  MessageHandler();
  virtual ~MessageHandler();

  // Posts a message to the target after a delay.
  // data may be nullptr if no extra data need to be sent. It will be deleted
  // automatically when the message is deleted.
  void PostMessageDelayed(TBID message, MessageData* data,
                          uint32_t delay_in_ms);

  // Posts a message to the target at the given time (relative to
  // util::GetTimeMS()).
  // data may be nullptr if no extra data need to be sent. It will be deleted
  // automatically when the message is deleted.
  void PostMessageOnTime(TBID message, MessageData* data, uint64_t fire_time);

  // Posts a message to the target.
  // data may be nullptr if no extra data need to be sent. It will be deleted
  // automatically when the message is deleted.
  void PostMessage(TBID message, MessageData* data);

  // Checks if this messagehandler has a pending message with the given id.
  // Returns the message if found, or nullptr.
  // If you want to delete the message, call DeleteMessage.
  Message* GetMessageByID(TBID message);

  // Deletes the message from this message handler.
  void DeleteMessage(Message* msg);

  // Deletes all messages from this message handler.
  void DeleteAllMessages();

  // Called when a message is delivered.
  // This message won't be found using GetMessageByID. It is already removed
  // from the list.
  // You should not call DeleteMessage on this message. That is done
  // automatically after this method exit.
  virtual void OnMessageReceived(Message* msg) {}

  // Processes any messages in queue.
  static void ProcessMessages();

  // Gets when the time when ProcessMessages needs to be called again.
  // Always returns 0 if there is nondelayed messages to process, which means it
  // needs to be called asap.
  // If there's only delayed messages to process, it returns the time that the
  // earliest delayed message should be fired.
  // If there's no more messages to process at the moment, it returns
  // kNotSoon (No call to ProcessMessages is needed). */
  static uint64_t GetNextMessageFireTime();

 private:
  util::TBLinkListOf<Message> m_messages;
};

}  // namespace tb

#endif  // TB_MSG_H
