/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_CLIPBOARD_H_
#define EL_UTIL_CLIPBOARD_H_

#include <string>

namespace el {
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
}  // namespace el

#endif  // EL_UTIL_CLIPBOARD_H_
