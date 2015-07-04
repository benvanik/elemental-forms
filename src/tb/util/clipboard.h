/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_CLIPBOARD_H_
#define TB_UTIL_CLIPBOARD_H_

#include <string>

namespace tb {
namespace util {

// A porting interface for the clipboard.
class Clipboard {
 public:
  // Empties the contents of the clipboard.
  static void Empty();

  // Returns true if the clipboard currently contains text.
  static bool HasText();

  // Gets the text from the clipboard in UTF-8 format.
  static std::string GetText();

  // Sets the text of the clipboard in UTF-8 format.
  static bool SetText(const std::string& text);
};

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_CLIPBOARD_H_
