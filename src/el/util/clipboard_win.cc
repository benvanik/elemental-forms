/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/util/clipboard.h"

#define NOMINMAX
#include <windows.h>

namespace el {
namespace util {

void Clipboard::Empty() {
  if (OpenClipboard(nullptr)) {
    EmptyClipboard();
    CloseClipboard();
  }
}

bool Clipboard::HasText() {
  bool has_text = false;
  if (OpenClipboard(nullptr)) {
    has_text = IsClipboardFormatAvailable(CF_TEXT) ||
               IsClipboardFormatAvailable(CF_OEMTEXT) ||
               IsClipboardFormatAvailable(CF_UNICODETEXT);
    CloseClipboard();
  }
  return has_text;
}

std::string Clipboard::GetText() {
  if (!HasText()) {
    return "";
  }
  if (!OpenClipboard(nullptr)) {
    return "";
  }
  HANDLE clipboard_data = GetClipboardData(CF_UNICODETEXT);
  if (!clipboard_data) {
    CloseClipboard();
    return "";
  }
  auto source_buffer = reinterpret_cast<wchar_t*>(GlobalLock(clipboard_data));
  int len =
      WideCharToMultiByte(CP_UTF8, 0, source_buffer, -1, NULL, 0, NULL, NULL);
  std::string result;
  result.reserve(len);
  result.resize(len - 1);
  WideCharToMultiByte(CP_UTF8, 0, source_buffer, -1,
                      const_cast<char*>(result.data()), len, NULL, NULL);
  GlobalUnlock(clipboard_data);
  CloseClipboard();
  return result;
}

bool Clipboard::SetText(const std::string& text) {
  if (!OpenClipboard(nullptr)) {
    return false;
  }

  int num_wide_chars_needed =
      MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
  if (HGLOBAL hClipboardData =
          GlobalAlloc(GMEM_DDESHARE, num_wide_chars_needed * sizeof(wchar_t))) {
    LPWSTR pchData = (LPWSTR)GlobalLock(hClipboardData);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pchData,
                        num_wide_chars_needed);
    GlobalUnlock(hClipboardData);

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hClipboardData);
  }

  CloseClipboard();
  return true;
}

}  // namespace util
}  // namespace el
