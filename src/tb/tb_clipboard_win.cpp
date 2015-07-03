/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_system.h"

#ifdef TB_CLIPBOARD_WINDOWS

#include <cstdio>

#define NOMINMAX
#include <Windows.h>

namespace tb {

void TBClipboard::Empty() {
  if (OpenClipboard(NULL)) {
    EmptyClipboard();
    CloseClipboard();
  }
}

bool TBClipboard::HasText() {
  bool has_text = false;
  if (OpenClipboard(NULL)) {
    has_text = IsClipboardFormatAvailable(CF_TEXT) ||
               IsClipboardFormatAvailable(CF_OEMTEXT) ||
               IsClipboardFormatAvailable(CF_UNICODETEXT);
    CloseClipboard();
  }
  return has_text;
}

bool TBClipboard::SetText(const std::string& text) {
  if (OpenClipboard(NULL)) {
    int num_wide_chars_needed =
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (HGLOBAL hClipboardData = GlobalAlloc(
            GMEM_DDESHARE, num_wide_chars_needed * sizeof(wchar_t))) {
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
  return false;
}

std::string TBClipboard::GetText() {
  std::string result;
  if (HasText() && OpenClipboard(NULL)) {
    if (HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT)) {
      wchar_t* pchData = (wchar_t*)GlobalLock(hClipboardData);
      int len =
          WideCharToMultiByte(CP_UTF8, 0, pchData, -1, NULL, 0, NULL, NULL);
      char* utf8 = new char[len];
      WideCharToMultiByte(CP_UTF8, 0, pchData, -1, utf8, len, NULL, NULL);
      result = utf8;
      delete[] utf8;
      GlobalUnlock(hClipboardData);
    }
    CloseClipboard();
  }
  return result;
}

}  // namespace tb

#endif  // TB_CLIPBOARD_WINDOWS
