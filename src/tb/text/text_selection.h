/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TEXT_TEXT_SELECTION_H_
#define TB_TEXT_TEXT_SELECTION_H_

#include "tb/color.h"

namespace tb {
namespace text {

class TextView;
class TextBlock;
class TextFragment;

class TextOffset {
 public:
  TextOffset() = default;
  TextOffset(TextBlock* block, size_t ofs) : block(block), ofs(ofs) {}

  void Set(TextBlock* new_block, size_t new_ofs) {
    block = new_block;
    ofs = new_ofs;
  }
  void Set(const TextOffset& pos) {
    block = pos.block;
    ofs = pos.ofs;
  }

  size_t GetGlobalOffset(TextView* se) const;
  bool SetGlobalOffset(TextView* se, size_t gofs);

 public:
  TextBlock* block = nullptr;
  size_t ofs = 0;
};

// Handles the selected text in a TextView.
class TextSelection {
 public:
  TextSelection(TextView* style_edit);
  void Invalidate() const;
  void Select(const TextOffset& new_start, const TextOffset& new_stop);
  void Select(const Point& from, const Point& to);
  void Select(size_t glob_ofs_from, size_t glob_ofs_to);
  void SelectToCaret(TextBlock* old_caret_block, size_t old_caret_ofs);
  void SelectAll();
  void SelectNothing();
  void CorrectOrder();
  void CopyToClipboard();
  bool IsBlockSelected(TextBlock* block) const;
  bool IsFragmentSelected(TextFragment* elm) const;
  bool IsSelected() const;
  void RemoveContent();
  std::string text() const;

 public:
  TextView* style_edit;
  TextOffset start;
  TextOffset stop;
};

}  // namespace text
}  // namespace tb

#endif  // TB_TEXT_TEXT_SELECTION_H_
