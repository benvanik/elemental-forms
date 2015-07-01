/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_system.h"

#ifdef TB_CLIPBOARD_GLFW

#include "GLFW/glfw3.h"

namespace tb {

void TBClipboard::Empty() { SetText(""); }

bool TBClipboard::HasText() {
  if (GLFWwindow* window = glfwGetCurrentContext()) {
    const char* str = glfwGetClipboardString(window);
    if (str && *str) return true;
  }
  return false;
}

bool TBClipboard::SetText(const char* text) {
  if (GLFWwindow* window = glfwGetCurrentContext()) {
    glfwSetClipboardString(window, text);
    return true;
  }
  return false;
}

TBStr TBClipboard::GetText() {
  text.clear();
  if (GLFWwindow* window = glfwGetCurrentContext()) {
    if (const char* str = glfwGetClipboardString(window)) {
      return str;
    }
  }
}

};  // namespace tb

#endif  // TB_CLIPBOARD_GLFW
