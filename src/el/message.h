/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_MESSAGE_H_
#define EL_MESSAGE_H_

#include <memory>

#include "el/id.h"
#include "el/util/intrusive_list.h"
#include "el/util/object.h"
#include "el/value.h"

namespace el {

class MessageHandler;

// Holds custom data to send with a posted message.
struct MessageData : public util::TypedObject {
 public:
  MessageData() = default;
  MessageData(int v1, int v2) : v1(v1), v2(v2) {}
  virtual ~MessageData() = default;

  // Values can be used for anything.
  Value v1;
  Value v2;
  TBID id1;
  TBID id2;
};

// Should never be created or subclassed anywhere except in Message.
// It's only purpose is to add a extra typed link for Message, since it needs
// to be added in multiple lists.
class MessageLink : public util::IntrusiveListEntry<MessageLink> {};

// A message created and owned by MessageHandler.
// It carries a message id, and may also carry a MessageData with additional
// parameters.
class Message : public util::IntrusiveListEntry<Message>, public MessageLink {
 public:
  Message(TBID message_id, std::unique_ptr<MessageData> data,
          uint64_t fire_time_millis, MessageHandler* message_handler)
      : message_id_(message_id),
        data_(std::move(data)),
        fire_time_millis_(fire_time_millis),
        message_handler_(message_handler) {}
  ~Message() = default;

  const TBID& message_id() const { return message_id_; }
  // The message data, or nullptr if no data is set.
  MessageData* data() const { return data_.get(); }
  // Gets the time which a delayed message should have fired (0 for non delayed
  // messages).
  uint64_t fire_time_millis() const { return fire_time_millis_; }
  MessageHandler* message_handler() const { return message_handler_; }

 private:
  TBID message_id_;
  std::unique_ptr<MessageData> data_;
  uint64_t fire_time_millis_;
  MessageHandler* message_handler_;
};

}  // namespace el

#endif  // EL_MESSAGE_H_
