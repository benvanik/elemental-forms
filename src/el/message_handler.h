/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_MESSAGE_HANDLER_H_
#define EL_MESSAGE_HANDLER_H_

#include <memory>

#include "el/id.h"
#include "el/message.h"
#include "el/util/intrusive_list.h"

namespace el {

// Handles a list of pending messages posted to itself.
// Messages can be delivered immediately or after a delay.
// Delayed message are delivered as close as possible to the time they should
// fire.
// Immediate messages are put on a queue and delivered as soon as possible,
// after any delayed messages that has passed their delivery time. This queue is
// global (among all MessageHandlers).
class MessageHandler {
 public:
  // kNotSoon is returned from MessageHandler::GetNextMessageFireTime
  // and means that there is currently no more messages to process.
  static const uint64_t kNotSoon = ~0ull;

  MessageHandler();
  virtual ~MessageHandler();

  // Posts a message to the target after a delay.
  // data may be nullptr if no extra data need to be sent. It will be deleted
  // automatically when the message is deleted.
  void PostMessageDelayed(TBID message_id, std::unique_ptr<MessageData> data,
                          uint32_t delay_in_ms);

  // Posts a message to the target at the given time (relative to
  // util::GetTimeMS()).
  // data may be nullptr if no extra data need to be sent. It will be deleted
  // automatically when the message is deleted.
  void PostMessageOnTime(TBID message_id, std::unique_ptr<MessageData> data,
                         uint64_t fire_time);

  // Posts a message to the target.
  // data may be nullptr if no extra data need to be sent. It will be deleted
  // automatically when the message is deleted.
  void PostMessage(TBID message_id, std::unique_ptr<MessageData> data);

  // Checks if this messagehandler has a pending message with the given id.
  // Returns the message if found, or nullptr.
  // If you want to delete the message, call DeleteMessage.
  Message* GetMessageById(TBID message_id);

  // Deletes the message from this message handler.
  void DeleteMessage(Message* msg);

  // Deletes all messages from this message handler.
  void DeleteAllMessages();

  // Called when a message is delivered.
  // This message won't be found using GetMessageById. It is already removed
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
  // MessageHandler::kNotSoon (no call to ProcessMessages is needed). */
  static uint64_t GetNextMessageFireTime();

 private:
  util::IntrusiveList<Message> m_messages;
};

}  // namespace el

#endif  // EL_MESSAGE_HANDLER_H_
