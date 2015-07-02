#pragma once

#include "tb_text_box.h"

#include <vector>

class CodeTextBox : public tb::TextBox {
 public:
  enum SpecialStringTypes { Keyword, Variable };
  CodeTextBox();

  virtual void OnInflate(const tb::InflateInfo& info);

 private:
  virtual void DrawString(int32_t x, int32_t y, tb::FontFace* font,
                          const tb::Color& color, const char* str, size_t len);
  virtual void OnBreak();

  bool StringHasColorOverride(const char* str, size_t len, tb::Color& colour);

  bool inComment;
};
