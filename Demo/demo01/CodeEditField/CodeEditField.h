#pragma once

#include "tb_editfield.h"

#include <vector>

class CodeEditField : public tb::TBEditField {
 public:
  enum SpecialStringTypes { Keyword, Variable };
  CodeEditField();

  virtual void OnInflate(const tb::INFLATE_INFO& info);

 private:
  virtual void DrawString(int32_t x, int32_t y, tb::TBFontFace* font,
                          const tb::TBColor& color, const char* str,
                          int32_t len);
  virtual void OnBreak();

  bool StringHasColorOverride(const char* str, int32_t len,
                              tb::TBColor& colour);

  bool inComment;
};