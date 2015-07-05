/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TEXT_CARET_H_
#define TB_TEXT_CARET_H_

#include <cstdint>

#include "tb/rect.h"
#include "tb/text/text_selection.h"

namespace tb {
namespace text {

class TextView;
class TextBlock;
class TextFragment;

enum class CaretPosition {
  kBeginning,
  kEnd,
};

// The caret in a TextView.
class Caret {
 public:
  Caret(TextView* style_edit);
  void Invalidate();
  void UpdatePos();
  bool Move(bool forward, bool word);
  bool Place(const Point& point);
  bool Place(TextBlock* block, size_t ofs, bool allow_snap = true,
             bool snap_forward = false);
  void Place(CaretPosition place);
  void AvoidLineBreak();
  void Paint(int32_t translate_x, int32_t translate_y);
  void ResetBlink();
  void UpdateWantedX();

  size_t global_offset() const { return pos.GetGlobalOffset(style_edit); }
  void set_global_offset(size_t gofs, bool allow_snap = true,
                         bool snap_forward = false);

  TextFragment* fragment();

 private:
  void SwitchBlock(bool second);

 public:
  TextView* style_edit;
  int32_t x = 0, y = 0;  // Relative to the style_edit.
  int32_t width = 2;
  int32_t height = 0;
  int32_t wanted_x = 0;
  bool on = false;
  bool prefer_first = true;
  TextOffset pos;
};

}  // namespace text
}  // namespace tb

#endif  // TB_TEXT_CARET_H_
