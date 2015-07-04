/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_STRING_TABLE_H_
#define TB_UTIL_STRING_TABLE_H_

#include <memory>
#include <unordered_map>

#include "tb_id.h"

namespace tb {
namespace util {

// A basic language string manager.
// Strings read into it can be looked up from a TBID, so either by a number or
// the hash number from a string (done by TBID).
//
//    Ex: GetString(10)      (Get the string with id 10)
//    Ex: GetString("new")   (Get the string with id new)
//
// In UI resources, you can refer to strings from language lookup by preceding a
// string with @.
//
//    Ex: Button: text: @close   (Create a button with text from lookup of
//                                "close")
class StringTable {
 public:
  static StringTable* get() { return string_table_singleton_.get(); }
  static void set(std::unique_ptr<StringTable> value) {
    string_table_singleton_ = std::move(value);
  }

  // Loads a file into this language manager.
  // NOTE: This *adds* strings read from the file, without clearing any existing
  // strings first.
  bool Load(const char* filename);

  // Clears the list of strings.
  void Clear();

  // Returns the string with the given id.
  // If there is no string with that id, "<TRANSLATE!>" will be returned
  // in release builds, and "<TRANSLATE:%s>" (populated with the id) will
  // be returned in debug builds.
  std::string GetString(const TBID& id);

 private:
  static std::unique_ptr<StringTable> string_table_singleton_;

  std::unordered_map<uint32_t, std::string> table_;
};

inline std::string GetString(const TBID& id) {
  return StringTable::get()->GetString(id);
}

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_STRING_TABLE_H_
