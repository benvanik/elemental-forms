/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef TESTBED_SCRATCH_CODE_TEXT_BOX_H_
#define TESTBED_SCRATCH_CODE_TEXT_BOX_H_

#include <vector>

#include "el/elements/text_box.h"

namespace testbed {

class CodeTextBox : public el::elements::TextBox {
 public:
  static void RegisterInflater();

  enum SpecialStringTypes { Keyword, Variable };
  CodeTextBox();

  virtual void OnInflate(const el::parsing::InflateInfo& info);

 private:
  virtual void DrawString(int32_t x, int32_t y, el::text::FontFace* font,
                          const el::Color& color, const char* str, size_t len);
  virtual void OnBreak();

  bool StringHasColorOverride(const char* str, size_t len, el::Color& colour);

  bool inComment;
};

}  // namespace testbed

#endif  // TESTBED_SCRATCH_CODE_TEXT_BOX_H_
