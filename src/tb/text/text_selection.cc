/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "tb/text/font_manager.h"
#include "tb/text/text_selection.h"
#include "tb/text/text_view.h"
#include "tb/util/clipboard.h"

namespace tb {
namespace text {

TextSelection::TextSelection(TextView* style_edit) : style_edit(style_edit) {}

void TextSelection::CorrectOrder() {
  if (start.block == stop.block && start.ofs == stop.ofs) {
    SelectNothing();
  } else {
    if ((start.block == stop.block && start.ofs > stop.ofs) ||
        (start.block != stop.block && start.block->ypos > stop.block->ypos)) {
      TextOffset tmp = start;
      start = stop;
      stop = tmp;
    }
  }
}

void TextSelection::CopyToClipboard() {
  if (IsSelected()) {
    util::Clipboard::SetText(GetText());
  }
}

void TextSelection::Invalidate() const {
  TextBlock* block = start.block;
  while (block) {
    block->Invalidate();
    if (block == stop.block) {
      break;
    }
    block = block->GetNext();
  }
}

void TextSelection::Select(const TextOffset& new_start,
                           const TextOffset& new_stop) {
  Invalidate();
  start.Set(new_start);
  stop.Set(new_stop);
  CorrectOrder();
  Invalidate();
}

void TextSelection::Select(const Point& from, const Point& to) {
  Invalidate();
  style_edit->caret.Place(from);
  start.Set(style_edit->caret.pos);
  style_edit->caret.Place(to);
  stop.Set(style_edit->caret.pos);
  CorrectOrder();
  Invalidate();
  style_edit->caret.UpdateWantedX();
}

void TextSelection::Select(size_t glob_ofs_from, size_t glob_ofs_to) {
  TextOffset ofs1, ofs2;
  ofs1.SetGlobalOffset(style_edit, glob_ofs_from);
  ofs2.SetGlobalOffset(style_edit, glob_ofs_to);
  Select(ofs1, ofs2);
}

void TextSelection::SelectToCaret(TextBlock* old_caret_block,
                                  size_t old_caret_ofs) {
  Invalidate();
  if (!start.block) {
    start.Set(old_caret_block, old_caret_ofs);
    stop.Set(style_edit->caret.pos);
  } else {
    if (start.block == old_caret_block && start.ofs == old_caret_ofs) {
      start.Set(style_edit->caret.pos);
    } else {
      stop.Set(style_edit->caret.pos);
    }
  }
  CorrectOrder();
  Invalidate();
}

void TextSelection::SelectAll() {
  start.Set(style_edit->blocks.GetFirst(), 0);
  stop.Set(style_edit->blocks.GetLast(), style_edit->blocks.GetLast()->str_len);
  Invalidate();
}

void TextSelection::SelectNothing() {
  Invalidate();
  start.Set(nullptr, 0);
  stop.Set(nullptr, 0);
}

bool TextSelection::IsBlockSelected(TextBlock* block) const {
  if (!IsSelected()) return false;
  return block->ypos >= start.block->ypos && block->ypos <= stop.block->ypos;
}

bool TextSelection::IsFragmentSelected(TextFragment* elm) const {
  if (!IsSelected()) return false;
  if (start.block == stop.block) {
    if (elm->block != start.block) {
      return false;
    }
    if (start.ofs < elm->ofs + elm->len && stop.ofs >= elm->ofs) {
      return true;
    }
    return false;
  }
  if (elm->block->ypos > start.block->ypos &&
      elm->block->ypos < stop.block->ypos) {
    return true;
  }
  if (elm->block->ypos == start.block->ypos &&
      elm->ofs + elm->len > start.ofs) {
    return true;
  }
  if (elm->block->ypos == stop.block->ypos && elm->ofs < stop.ofs) {
    return true;
  }
  return false;
}

bool TextSelection::IsSelected() const { return start.block ? true : false; }

void TextSelection::RemoveContent() {
  if (!IsSelected()) return;
  style_edit->BeginLockScrollbars();
  if (start.block == stop.block) {
    if (!style_edit->undo_stack.applying) {
      style_edit->undo_stack.Commit(
          style_edit, start.GetGlobalOffset(style_edit), stop.ofs - start.ofs,
          start.block->str.c_str() + start.ofs, false);
    }
    start.block->RemoveContent(start.ofs, stop.ofs - start.ofs);
  } else {
    // Remove text in first block.
    util::StringBuilder commit_string;
    size_t start_gofs = 0;
    if (!style_edit->undo_stack.applying) {
      start_gofs = start.GetGlobalOffset(style_edit);
      commit_string.Append(start.block->str.c_str() + start.ofs,
                           start.block->str_len - start.ofs);
    }
    start.block->RemoveContent(start.ofs, start.block->str_len - start.ofs);

    // Remove text in all block in between start and stop.
    TextBlock* block = start.block->GetNext();
    while (block != stop.block) {
      if (!style_edit->undo_stack.applying) {
        commit_string.Append(block->str, block->str_len);
      }

      TextBlock* next = block->GetNext();
      style_edit->blocks.Delete(block);
      block = next;
    }

    // Remove text in last block.
    if (!style_edit->undo_stack.applying) {
      commit_string.Append(stop.block->str, stop.ofs);
      style_edit->undo_stack.Commit(style_edit, start_gofs,
                                    commit_string.GetAppendPos(),
                                    commit_string.GetData(), false);
    }
    stop.block->RemoveContent(0, stop.ofs);
  }
  stop.block->Merge();
  start.block->Merge();
  style_edit->caret.Place(start.block, start.ofs);
  style_edit->caret.UpdateWantedX();
  SelectNothing();
  style_edit->EndLockScrollbars();
}

std::string TextSelection::GetText() const {
  if (!IsSelected()) {
    return std::string();
  }
  if (start.block == stop.block) {
    return std::string(start.block->str.c_str() + start.ofs,
                       stop.ofs - start.ofs);
  } else {
    util::StringBuilder buf;
    buf.Append(start.block->str.c_str() + start.ofs,
               start.block->str_len - start.ofs);
    TextBlock* block = start.block->GetNext();
    while (block != stop.block) {
      buf.Append(block->str, block->str_len);
      block = block->GetNext();
    }
    // FIX: Add methods to change data owner from temp buffer to string!
    buf.Append(stop.block->str, stop.ofs);
    return std::string(buf.GetData(), buf.GetAppendPos());
  }
}

size_t TextOffset::GetGlobalOffset(TextView* se) const {
  size_t gofs = 0;
  TextBlock* b = se->blocks.GetFirst();
  while (b && b != block) {
    gofs += b->str_len;
    b = b->GetNext();
  }
  gofs += ofs;
  return gofs;
}

bool TextOffset::SetGlobalOffset(TextView* se, size_t gofs) {
  TextBlock* b = se->blocks.GetFirst();
  while (b) {
    size_t b_len = b->str_len;
    if (gofs <= b_len) {
      block = b;
      ofs = gofs;
      return true;
    }
    gofs -= b_len;
    b = b->GetNext();
  }
  assert(!"out of range! not a valid global offset!");
  return false;
}

}  // namespace text
}  // namespace tb
