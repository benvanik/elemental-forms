/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/text/caret.h"
#include "el/text/text_view.h"
#include "el/text/utf8.h"
#include "el/util/string.h"

namespace el {
namespace text {

Caret::Caret(TextView* style_edit) : style_edit(style_edit) {}

void Caret::Invalidate() {
  if (style_edit->listener)
    style_edit->listener->Invalidate(Rect(
        x - style_edit->scroll_x, y - style_edit->scroll_y, width, height));
}

void Caret::UpdatePos() {
  Invalidate();
  TextFragment* fragment = this->fragment();
  x = fragment->xpos +
      fragment->GetCharX(style_edit->font, pos.ofs - fragment->ofs);
  y = fragment->ypos + pos.block->ypos;
  height = fragment->GetHeight(style_edit->font);
  if (!height) {
    // If we don't have height, we're probably inside a style switch embed.
    y = fragment->line_ypos + pos.block->ypos;
    height = fragment->line_height;
  }
  Invalidate();
}

bool Caret::Move(bool forward, bool word) {
  // Make it stay on the same line if it reach the wrap point.
  prefer_first = forward;
  if (this->style_edit->packed.password_on) {
    word = false;
  }

  size_t len = pos.block->str_len;
  if (word && !(forward && pos.ofs == len) && !(!forward && pos.ofs == 0)) {
    const char* str = pos.block->str.c_str();
    if (forward) {
      if (util::is_linebreak(str[pos.ofs])) {
        pos.ofs++;
      } else if (util::is_wordbreak(str[pos.ofs])) {
        while (pos.ofs < len && util::is_wordbreak(str[pos.ofs]) &&
               !util::is_linebreak(str[pos.ofs]))
          pos.ofs++;
      } else {
        while (pos.ofs < len && !util::is_wordbreak(str[pos.ofs])) pos.ofs++;
        while (pos.ofs < len && util::is_space(str[pos.ofs])) pos.ofs++;
      }
    } else if (pos.ofs > 0) {
      while (pos.ofs > 0 && util::is_space(str[pos.ofs - 1])) pos.ofs--;
      if (pos.ofs > 0 && util::is_wordbreak(str[pos.ofs - 1])) {
        while (pos.ofs > 0 && util::is_wordbreak(str[pos.ofs - 1])) pos.ofs--;
      } else {
        while (pos.ofs > 0 && !util::is_wordbreak(str[pos.ofs - 1])) pos.ofs--;
      }
    }
  } else {
    if (forward && pos.ofs >= pos.block->str_len && pos.block->GetNext()) {
      pos.block = pos.block->GetNext();
      pos.ofs = 0;
    } else if (!forward && pos.ofs <= 0 && pos.block->prev) {
      pos.block = pos.block->GetPrev();
      pos.ofs = pos.block->str_len;
    } else {
      size_t i = pos.ofs;
      if (forward) {
        utf8::move_inc(pos.block->str.c_str(), &i, pos.block->str_len);
      } else {
        utf8::move_dec(pos.block->str.c_str(), &i);
      }
      pos.ofs = i;
    }
  }
  return Place(pos.block, pos.ofs, true, forward);
}

bool Caret::Place(const Point& point) {
  TextBlock* block = style_edit->FindBlock(point.y);
  TextFragment* fragment = block->FindFragment(point.x, point.y - block->ypos);
  size_t ofs = fragment->ofs +
               fragment->GetCharOfs(style_edit->font, point.x - fragment->xpos);

  if (Place(block, ofs)) {
    if (this->fragment() != fragment) {
      prefer_first = !prefer_first;
      Place(block, ofs);
    }
    return true;
  }
  return false;
}

void Caret::Place(CaretPosition place) {
  if (place == CaretPosition::kBeginning) {
    Place(style_edit->blocks.GetFirst(), 0);
  } else if (place == CaretPosition::kEnd) {
    Place(style_edit->blocks.GetLast(), style_edit->blocks.GetLast()->str_len);
  }
}

bool Caret::Place(TextBlock* block, size_t ofs, bool allow_snap,
                  bool snap_forward) {
  if (block) {
    while (block->GetNext() && ofs > block->str_len) {
      ofs -= block->str_len;
      block = block->GetNext();
    }
    while (block->prev) {
      block = block->GetPrev();
      ofs += block->str_len;
    }
    if (ofs > block->str_len) {
      ofs = block->str_len;
    }

    // Avoid being inside linebreak.
    if (allow_snap) {
      TextFragment* fragment = block->FindFragment(ofs);
      if (ofs > fragment->ofs && fragment->IsBreak()) {
        if (snap_forward && block->GetNext()) {
          block = block->GetNext();
          ofs = 0;
        } else {
          ofs = fragment->ofs;
        }
      }
    }
  }

  bool changed = (pos.block != block || pos.ofs != ofs);
  pos.Set(block, ofs);

  if (block) {
    UpdatePos();
  }
  return changed;
}

void Caret::AvoidLineBreak() {
  TextFragment* fragment = this->fragment();
  if (pos.ofs > fragment->ofs && fragment->IsBreak()) {
    pos.ofs = fragment->ofs;
  }
  UpdatePos();
}

void Caret::Paint(int32_t translate_x, int32_t translate_y) {
  //	if (on && !(style_edit->select_state &&
  // style_edit->selection.IsSelected()))
  if (on || style_edit->select_state) {
    style_edit->listener->DrawCaret(
        Rect(translate_x + x, translate_y + y, width, height));
  }
}

void Caret::ResetBlink() {
  style_edit->listener->CaretBlinkStop();
  on = true;
  style_edit->listener->CaretBlinkStart();
}

void Caret::UpdateWantedX() { wanted_x = x; }

TextFragment* Caret::fragment() {
  return pos.block->FindFragment(pos.ofs, prefer_first);
}

void Caret::SwitchBlock(bool second) {}

void Caret::set_global_offset(size_t gofs, bool allow_snap, bool snap_forward) {
  TextOffset ofs;
  if (ofs.SetGlobalOffset(style_edit, gofs)) {
    Place(ofs.block, ofs.ofs, allow_snap, snap_forward);
  }
}

}  // namespace text
}  // namespace el
