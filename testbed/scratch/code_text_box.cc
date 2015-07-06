/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * �2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cctype>

#include "el/parsing/element_factory.h"
#include "el/parsing/element_inflater.h"
#include "testbed/scratch/code_text_box.h"

namespace testbed {

using namespace el;

void CodeTextBox::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(CodeTextBox, Value::Type::kString,
                               ElementZ::kTop);
}

CodeTextBox::CodeTextBox() : TextBox(), inComment(false) {}

void CodeTextBox::OnInflate(const parsing::InflateInfo& info) {
  TextBox::OnInflate(info);
}

void CodeTextBox::DrawString(int32_t x, int32_t y, text::FontFace* font,
                             const Color& color, const char* str, size_t len) {
  Color finalColor(color);
  StringHasColorOverride(str, len, finalColor);
  TextBox::DrawString(x, y, font, finalColor, str, len);
}

void CodeTextBox::OnBreak() { inComment = false; }

bool CodeTextBox::StringHasColorOverride(const char* str, size_t len,
                                         Color& colour) {
  if (strlen(str) >= 2) {
    if (str[0] == '/' && str[1] == '/') {
      inComment = true;
    }
  }

  if (inComment) {
    colour = Color(113, 143, 113);
    return true;
  }

  char* keywords[] = {"in",  "vec3",  "uvec2",   "const", "uniform", "void",
                      "if",  "float", "vec4",    "for",   "uint",    "abs",
                      "sin", "cos",   "texture", "int"};

  for (int32_t keywordIdx = 0; keywordIdx < 16; keywordIdx++) {
    char* matchAgainst = keywords[keywordIdx];
    int32_t matchLen = (int32_t)strlen(matchAgainst);
    if (matchLen == len) {
      auto matched = true;
      for (int32_t i = 0; i < len; ++i) {
        if (toupper(matchAgainst[i]) != toupper(str[i])) {
          matched = false;
          break;
        }
      }

      if (matched) {
        colour = Color(90, 127, 230);
        return true;
      }
    }
  }

  return false;
}

}  // namespace testbed
