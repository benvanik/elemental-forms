/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/text/text_view.h"
#include "tb/text/undo_stack.h"
#include "tb/text/utf8.h"

namespace tb {
namespace text {

void UndoStack::Undo(TextView* style_edit) {
  if (undos.empty()) return;
  auto e = std::move(undos.back());
  undos.pop_back();
  auto e_ptr = e.get();
  redos.push_back(std::move(e));
  Apply(style_edit, e_ptr, true);
}

void UndoStack::Redo(TextView* style_edit) {
  if (redos.empty()) return;
  auto e = std::move(redos.back());
  redos.pop_back();
  auto e_ptr = e.get();
  undos.push_back(std::move(e));
  Apply(style_edit, e_ptr, false);
}

void UndoStack::Apply(TextView* style_edit, UndoEvent* e, bool reverse) {
  applying = true;
  if (e->insert == reverse) {
    style_edit->selection.SelectNothing();
    style_edit->caret.SetGlobalOffset(e->gofs, false);
    assert(TextOffset(style_edit->caret.pos).GetGlobalOffset(style_edit) ==
           e->gofs);

    TextOffset start = style_edit->caret.pos;
    style_edit->caret.SetGlobalOffset(e->gofs + e->text.size(), false);
    assert(TextOffset(style_edit->caret.pos).GetGlobalOffset(style_edit) ==
           e->gofs + e->text.size());

    style_edit->selection.Select(start, style_edit->caret.pos);
    style_edit->selection.RemoveContent();
  } else {
    style_edit->selection.SelectNothing();
    style_edit->caret.SetGlobalOffset(e->gofs, true, true);
    style_edit->InsertText(e->text);
    size_t text_len = e->text.size();
    if (text_len > 1) {
      style_edit->selection.Select(e->gofs, e->gofs + text_len);
    }
  }
  style_edit->ScrollIfNeeded(true, true);
  applying = false;
}

void UndoStack::Clear(bool clear_undo, bool clear_redo) {
  assert(!applying);
  if (clear_undo) {
    undos.clear();
  }
  if (clear_redo) {
    redos.clear();
  }
}

UndoEvent* UndoStack::Commit(TextView* style_edit, size_t gofs, size_t len,
                             const char* text, bool insert) {
  if (applying || style_edit->packed.read_only) {
    return nullptr;
  }
  Clear(false, true);

  // If we're inserting a single character, check if we want to append it to the
  // previous event.
  if (insert && !undos.empty()) {
    size_t num_char = utf8::count_characters(text, len);
    auto e = undos.back().get();
    if (num_char == 1 && e->insert && e->gofs + e->text.size() == gofs) {
      // Appending a space to other space(s) should append.
      if ((text[0] == ' ' && !strpbrk(e->text.c_str(), "\r\n")) ||
          // But non spaces should not.
          !strpbrk(e->text.c_str(), " \r\n")) {
        e->text.append(text, len);
        return e;
      }
    }
  }

  // Create a new event.
  auto e = std::make_unique<UndoEvent>();
  e->gofs = gofs;
  e->text.assign(text, len);
  e->insert = insert;
  undos.push_back(std::move(e));
  return undos.back().get();
}

}  // namespace text
}  // namespace tb
