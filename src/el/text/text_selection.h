/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_TEXT_SELECTION_H_
#define EL_TEXT_TEXT_SELECTION_H_

#include <string>

#include "el/color.h"

namespace el {
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
  explicit TextSelection(TextView* style_edit);
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
}  // namespace el

#endif  // EL_TEXT_TEXT_SELECTION_H_
