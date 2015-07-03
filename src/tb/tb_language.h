/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_LANGUAGE_H
#define TB_LANGUAGE_H

#include <unordered_map>

#include "tb_core.h"
#include "tb_id.h"

namespace tb {

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
class Language {
 public:
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
  std::unordered_map<uint32_t, std::string> table_;
};

}  // namespace tb

#endif  // TB_LANGUAGE_H
