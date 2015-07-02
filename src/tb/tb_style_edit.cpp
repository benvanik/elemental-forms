/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_style_edit.h"

#include <algorithm>
#include <cassert>

#include "tb_font_renderer.h"
#include "tb_style_edit_content.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"
#include "tb_widgets_common.h"
#include "utf8.h"

namespace tb {

#if 0  // Enable for some graphical debugging
#define TMPDEBUG(expr) expr
#define nTMPDEBUG(expr)
#else
#define TMPDEBUG(expr)
#define nTMPDEBUG(expr) expr
#endif

const int TAB_SPACE = 4;

const char* special_char_newln = "¶";  // 00B6 PILCROW SIGN
const char* special_char_space = "·";  // 00B7 MIDDLE DOT
const char* special_char_tab =
    "»";  // 00BB RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
const char* special_char_password = "•";  // 2022 BULLET

static bool is_space(int8_t c) {
  switch (c) {
    case ' ':
      return true;
  }
  return false;
}

static bool is_linebreak(int8_t c) {
  switch (c) {
    case '\n':
    case '\r':
    case 0:
      return true;
  }
  return false;
}

static bool is_wordbreak(int8_t c) {
  switch (c) {
    case 0:
    case '\n':
    case '\r':
    case '-':
    case '\t':
    case '\"':
    case '(':
    case ')':
    case '/':
    case '\\':
    case '*':
    case '+':
    case ',':
    case '.':
    case ';':
    case ':':
    case '>':
    case '<':
    case '&':
    case '#':
    case '!':
    case '=':
    case '[':
    case ']':
    case '{':
    case '}':
    case '^':
      return true;
  }
  return is_space(c);
}

/** Check if no line wrapping is allowed before the character at the given
   offset.
        The string must be null terminated. */
static bool is_never_break_before(const char* str, int ofs) {
  switch (str[ofs]) {
    case '\n':
    case '\r':
    case ' ':
    case '-':
    case '.':
    case ',':
    case ':':
    case ';':
    case '!':
    case '?':
    case ')':
    case ']':
    case '}':
    case '>':
      return true;
    case '\'':
    case '"':
      // Simple test if it's the first quote in a word surrounded by space.
      if (ofs > 0 && !is_space(str[ofs - 1])) return true;
    default:
      return false;
  }
}

/** Check if no line wrapping is allowed after the character at the given
   offset.
        The string must be null terminated. */
static bool is_never_break_after(const char* str, int ofs) {
  switch (str[ofs]) {
    case '(':
    case '[':
    case '{':
    case '<':
      return true;
    case '\'':
    case '"':
      // Simple test if it's the last quote in a word surrounded by space.
      if (!is_space(str[ofs + 1])) return true;
    default:
      return false;
  }
}

static bool GetNextFragment(const char* text,
                            TBTextFragmentContentFactory* content_factory,
                            int* frag_len, bool* is_embed) {
  if (text[0] == '\t') {
    *frag_len = 1;
    return text[1] != 0;
  } else if (text[0] ==
             0)  // happens when not setting text and maby when setting ""
  {
    *frag_len = 0;
    return false;
  } else if (text[0] == '\r' || text[0] == '\n') {
    int len = (text[0] == '\r' && text[1] == '\n') ? 2 : 1;
    *frag_len = len;
    return false;
  } else if (content_factory) {
    if (int content_len = content_factory->GetContent(text)) {
      *frag_len = content_len;
      *is_embed = true;
      return text[content_len] != 0;
    }
  }
  int i = 0;
  while (!is_wordbreak(text[i])) i++;
  if (i == 0)
    if (is_wordbreak(text[i])) i++;
  *frag_len = i;
  if (text[i] == 0) return false;
  return true;
}

TBSelection::TBSelection(TBStyleEdit* styledit) : styledit(styledit) {}

void TBSelection::CorrectOrder() {
  if (start.block == stop.block && start.ofs == stop.ofs)
    SelectNothing();
  else {
    if ((start.block == stop.block && start.ofs > stop.ofs) ||
        (start.block != stop.block && start.block->ypos > stop.block->ypos)) {
      TBTextOfs tmp = start;
      start = stop;
      stop = tmp;
    }
  }
}

void TBSelection::CopyToClipboard() {
  if (IsSelected()) {
    TBClipboard::SetText(GetText());
  }
}

void TBSelection::Invalidate() const {
  TBBlock* block = start.block;
  while (block) {
    block->Invalidate();
    if (block == stop.block) break;
    block = block->GetNext();
  }
}

void TBSelection::Select(const TBTextOfs& new_start,
                         const TBTextOfs& new_stop) {
  Invalidate();
  start.Set(new_start);
  stop.Set(new_stop);
  CorrectOrder();
  Invalidate();
}

void TBSelection::Select(const Point& from, const Point& to) {
  Invalidate();
  styledit->caret.Place(from);
  start.Set(styledit->caret.pos);
  styledit->caret.Place(to);
  stop.Set(styledit->caret.pos);
  CorrectOrder();
  Invalidate();
  styledit->caret.UpdateWantedX();
}

void TBSelection::Select(size_t glob_ofs_from, size_t glob_ofs_to) {
  TBTextOfs ofs1, ofs2;
  ofs1.SetGlobalOfs(styledit, glob_ofs_from);
  ofs2.SetGlobalOfs(styledit, glob_ofs_to);
  Select(ofs1, ofs2);
}

void TBSelection::SelectToCaret(TBBlock* old_caret_block,
                                size_t old_caret_ofs) {
  Invalidate();
  if (!start.block) {
    start.Set(old_caret_block, old_caret_ofs);
    stop.Set(styledit->caret.pos);
  } else {
    if (start.block == old_caret_block && start.ofs == old_caret_ofs)
      start.Set(styledit->caret.pos);
    else
      stop.Set(styledit->caret.pos);
  }
  CorrectOrder();
  Invalidate();
}

void TBSelection::SelectAll() {
  start.Set(styledit->blocks.GetFirst(), 0);
  stop.Set(styledit->blocks.GetLast(), styledit->blocks.GetLast()->str_len);
  Invalidate();
}

void TBSelection::SelectNothing() {
  Invalidate();
  start.Set(nullptr, 0);
  stop.Set(nullptr, 0);
}

bool TBSelection::IsBlockSelected(TBBlock* block) const {
  if (!IsSelected()) return false;
  return block->ypos >= start.block->ypos && block->ypos <= stop.block->ypos;
}

bool TBSelection::IsFragmentSelected(TBTextFragment* elm) const {
  if (!IsSelected()) return false;
  if (start.block == stop.block) {
    if (elm->block != start.block) return false;
    if (start.ofs < elm->ofs + elm->len && stop.ofs >= elm->ofs) return true;
    return false;
  }
  if (elm->block->ypos > start.block->ypos &&
      elm->block->ypos < stop.block->ypos)
    return true;
  if (elm->block->ypos == start.block->ypos && elm->ofs + elm->len > start.ofs)
    return true;
  if (elm->block->ypos == stop.block->ypos && elm->ofs < stop.ofs) return true;
  return false;
}

bool TBSelection::IsSelected() const { return start.block ? true : false; }

void TBSelection::RemoveContent() {
  if (!IsSelected()) return;
  styledit->BeginLockScrollbars();
  if (start.block == stop.block) {
    if (!styledit->undoredo.applying)
      styledit->undoredo.Commit(styledit, start.GetGlobalOfs(styledit),
                                stop.ofs - start.ofs,
                                start.block->str.c_str() + start.ofs, false);
    start.block->RemoveContent(start.ofs, stop.ofs - start.ofs);
  } else {
    // Remove text in first block
    TBTempBuffer commit_string;
    size_t start_gofs = 0;
    if (!styledit->undoredo.applying) {
      start_gofs = start.GetGlobalOfs(styledit);
      commit_string.Append(start.block->str.c_str() + start.ofs,
                           start.block->str_len - start.ofs);
    }
    start.block->RemoveContent(start.ofs, start.block->str_len - start.ofs);

    // Remove text in all block in between start and stop
    TBBlock* block = start.block->GetNext();
    while (block != stop.block) {
      if (!styledit->undoredo.applying) {
        commit_string.Append(block->str, block->str_len);
      }

      TBBlock* next = block->GetNext();
      styledit->blocks.Delete(block);
      block = next;
    }

    // Remove text in last block
    if (!styledit->undoredo.applying) {
      commit_string.Append(stop.block->str, stop.ofs);
      styledit->undoredo.Commit(styledit, start_gofs,
                                commit_string.GetAppendPos(),
                                commit_string.GetData(), false);
    }
    stop.block->RemoveContent(0, stop.ofs);
  }
  stop.block->Merge();
  start.block->Merge();
  styledit->caret.Place(start.block, start.ofs);
  styledit->caret.UpdateWantedX();
  SelectNothing();
  styledit->EndLockScrollbars();
}

std::string TBSelection::GetText() const {
  if (!IsSelected()) {
    return std::string();
  }
  if (start.block == stop.block) {
    return std::string(start.block->str.c_str() + start.ofs,
                       stop.ofs - start.ofs);
  } else {
    TBTempBuffer buf;
    buf.Append(start.block->str.c_str() + start.ofs,
               start.block->str_len - start.ofs);
    TBBlock* block = start.block->GetNext();
    while (block != stop.block) {
      buf.Append(block->str, block->str_len);
      block = block->GetNext();
    }
    // FIX: Add methods to change data owner from temp buffer to string!
    buf.Append(stop.block->str, stop.ofs);
    return std::string(buf.GetData(), buf.GetAppendPos());
  }
}

size_t TBTextOfs::GetGlobalOfs(TBStyleEdit* se) const {
  size_t gofs = 0;
  TBBlock* b = se->blocks.GetFirst();
  while (b && b != block) {
    gofs += b->str_len;
    b = b->GetNext();
  }
  gofs += ofs;
  return gofs;
}

bool TBTextOfs::SetGlobalOfs(TBStyleEdit* se, size_t gofs) {
  TBBlock* b = se->blocks.GetFirst();
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

TBCaret::TBCaret(TBStyleEdit* styledit)
    : styledit(styledit),
      x(0),
      y(0),
      width(2),
      height(0),
      wanted_x(0),
      on(false),
      prefer_first(true) {}

void TBCaret::Invalidate() {
  if (styledit->listener)
    styledit->listener->Invalidate(
        Rect(x - styledit->scroll_x, y - styledit->scroll_y, width, height));
}

void TBCaret::UpdatePos() {
  Invalidate();
  TBTextFragment* fragment = GetFragment();
  x = fragment->xpos +
      fragment->GetCharX(styledit->font, pos.ofs - fragment->ofs);
  y = fragment->ypos + pos.block->ypos;
  height = fragment->GetHeight(styledit->font);
  if (!height) {
    // If we don't have height, we're probably inside a style switch embed.
    y = fragment->line_ypos + pos.block->ypos;
    height = fragment->line_height;
  }
  Invalidate();
}

bool TBCaret::Move(bool forward, bool word) {
  // Make it stay on the same line if it reach the wrap point.
  prefer_first = forward;
  if (this->styledit->packed.password_on) word = false;

  size_t len = pos.block->str_len;
  if (word && !(forward && pos.ofs == len) && !(!forward && pos.ofs == 0)) {
    const char* str = pos.block->str.c_str();
    if (forward) {
      if (is_linebreak(str[pos.ofs])) {
        pos.ofs++;
      } else if (is_wordbreak(str[pos.ofs])) {
        while (pos.ofs < len && is_wordbreak(str[pos.ofs]) &&
               !is_linebreak(str[pos.ofs]))
          pos.ofs++;
      } else {
        while (pos.ofs < len && !is_wordbreak(str[pos.ofs])) pos.ofs++;
        while (pos.ofs < len && is_space(str[pos.ofs])) pos.ofs++;
      }
    } else if (pos.ofs > 0) {
      while (pos.ofs > 0 && is_space(str[pos.ofs - 1])) pos.ofs--;
      if (pos.ofs > 0 && is_wordbreak(str[pos.ofs - 1])) {
        while (pos.ofs > 0 && is_wordbreak(str[pos.ofs - 1])) pos.ofs--;
      } else {
        while (pos.ofs > 0 && !is_wordbreak(str[pos.ofs - 1])) pos.ofs--;
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
      if (forward)
        utf8::move_inc(pos.block->str.c_str(), &i, pos.block->str_len);
      else
        utf8::move_dec(pos.block->str.c_str(), &i);
      pos.ofs = i;
    }
  }
  return Place(pos.block, pos.ofs, true, forward);
}

bool TBCaret::Place(const Point& point) {
  TBBlock* block = styledit->FindBlock(point.y);
  TBTextFragment* fragment =
      block->FindFragment(point.x, point.y - block->ypos);
  size_t ofs = fragment->ofs +
               fragment->GetCharOfs(styledit->font, point.x - fragment->xpos);

  if (Place(block, ofs)) {
    if (GetFragment() != fragment) {
      prefer_first = !prefer_first;
      Place(block, ofs);
    }
    return true;
  }
  return false;
}

void TBCaret::Place(CaretPosition place) {
  if (place == CaretPosition::kBeginning)
    Place(styledit->blocks.GetFirst(), 0);
  else if (place == CaretPosition::kEnd)
    Place(styledit->blocks.GetLast(), styledit->blocks.GetLast()->str_len);
}

bool TBCaret::Place(TBBlock* block, size_t ofs, bool allow_snap,
                    bool snap_forward) {
  if (block) {
    while (block->GetNext() && ofs > block->str_len) {
      ofs -= block->str_len;
      block = block->GetNext();
    }
    while (block->prev && ofs < 0) {
      block = block->GetPrev();
      ofs += block->str_len;
    }
    if (ofs < 0) ofs = 0;
    if (ofs > block->str_len) ofs = block->str_len;

    // Avoid being inside linebreak
    if (allow_snap) {
      TBTextFragment* fragment = block->FindFragment(ofs);
      if (ofs > fragment->ofs && fragment->IsBreak()) {
        if (snap_forward && block->GetNext()) {
          block = block->GetNext();
          ofs = 0;
        } else
          ofs = fragment->ofs;
      }
    }
  }

  bool changed = (pos.block != block || pos.ofs != ofs);
  pos.Set(block, ofs);

  if (block) UpdatePos();

  return changed;
}

void TBCaret::AvoidLineBreak() {
  TBTextFragment* fragment = GetFragment();
  if (pos.ofs > fragment->ofs && fragment->IsBreak()) pos.ofs = fragment->ofs;
  UpdatePos();
}

void TBCaret::Paint(int32_t translate_x, int32_t translate_y) {
  //	if (on && !(styledit->select_state && styledit->selection.IsSelected()))
  if (on || styledit->select_state) {
    styledit->listener->DrawCaret(
        Rect(translate_x + x, translate_y + y, width, height));
  }
}

void TBCaret::ResetBlink() {
  styledit->listener->CaretBlinkStop();
  on = true;
  styledit->listener->CaretBlinkStart();
}

void TBCaret::UpdateWantedX() { wanted_x = x; }

TBTextFragment* TBCaret::GetFragment() {
  return pos.block->FindFragment(pos.ofs, prefer_first);
}

void TBCaret::SwitchBlock(bool second) {}

void TBCaret::SetGlobalOfs(size_t gofs, bool allow_snap, bool snap_forward) {
  TBTextOfs ofs;
  if (ofs.SetGlobalOfs(styledit, gofs))
    Place(ofs.block, ofs.ofs, allow_snap, snap_forward);
}

TBTextProps::TBTextProps(const FontDescription& font_desc,
                         const Color& text_color) {
  base_data.font_desc = font_desc;
  base_data.text_color = text_color;
  base_data.underline = false;
  data = &base_data;
}

TBTextProps::Data* TBTextProps::Push() {
  if (Data* new_data = new Data) {
    data_list.AddLast(new_data);
    new_data->font_desc = data->font_desc;
    new_data->text_color = data->text_color;
    new_data->underline = data->underline;
    data = new_data;
    return data;
  }
  return nullptr;
}

void TBTextProps::Pop() {
  if (!data_list.GetLast()) return;  // Unballanced or we previosly got OOM.
  data_list.Delete(data_list.GetLast());
  data = data_list.GetLast() ? data_list.GetLast() : &base_data;
}

FontFace* TBTextProps::GetFont() {
  return g_font_manager->GetFontFace(data->font_desc);
}

TBBlock::TBBlock(TBStyleEdit* styledit)
    : styledit(styledit),
      ypos(0),
      height(0),
      align(int8_t(styledit->align)),
      line_width_max(0),
      str_len(0) {}

TBBlock::~TBBlock() { Clear(); }

void TBBlock::Clear() { fragments.DeleteAll(); }

void TBBlock::Set(const char* newstr, size_t len) {
  str.assign(newstr, len);
  str_len = len;
  Split();
  Layout(true, true);
}

void TBBlock::SetAlign(TextAlign align) {
  if (TextAlign(this->align) == align) return;
  this->align = int8_t(align);
  Layout(false, false);
}

size_t TBBlock::InsertText(size_t ofs, const char* text, size_t len,
                           bool allow_line_recurse) {
  styledit->BeginLockScrollbars();
  size_t first_line_len = len;
  for (size_t i = 0; i < len; i++)
    if (text[i] == '\r' || text[i] == '\n') {
      first_line_len = i;
      // Include the line break too but not for single lines
      if (!styledit->packed.multiline_on) break;
      if (text[i] == '\r' && text[i + 1] == '\n') first_line_len++;
      first_line_len++;
      break;
    }

  size_t inserted_len = first_line_len;
  str.insert(ofs, text, first_line_len);
  str_len += first_line_len;

  Split();
  Layout(true, true);

  // Add the rest which was after the linebreak.
  if (allow_line_recurse && styledit->packed.multiline_on) {
    // Instead of recursively calling InsertText, we will loop through them all
    // here
    TBBlock* next_block = GetNext();
    const char* next_line_ptr = &text[first_line_len];
    size_t remaining = len - first_line_len;
    while (remaining > 0) {
      if (!next_block) {
        next_block = new TBBlock(styledit);
        styledit->blocks.AddLast(next_block);
      }
      size_t consumed =
          next_block->InsertText(0, next_line_ptr, remaining, false);
      next_line_ptr += consumed;
      inserted_len += consumed;
      remaining -= consumed;
      next_block = next_block->GetNext();
    }
  }
  styledit->EndLockScrollbars();
  return inserted_len;
}

void TBBlock::RemoveContent(size_t ofs, size_t len) {
  if (!len) return;
  str.erase(ofs, len);
  str_len -= len;
  Layout(true, true);
}

void TBBlock::Split() {
  if (!str_len) {
    return;
  }
  size_t len = str_len;
  size_t brlen = 1;  // FIX: skip ending newline fragment but not if there is
                     // several newlines and check for singleline newline.
  if (len > 1 && str[len - 2] == '\r' && str[len - 1] == '\n') brlen++;
  len -= brlen;
  for (size_t i = 0; i < len; i++) {
    if (is_linebreak(str[i])) {
      TBBlock* block = new TBBlock(styledit);
      if (!block) return;
      styledit->blocks.AddAfter(block, this);

      if (i < len - 1 && str[i] == '\r' && str[i + 1] == '\n') i++;
      i++;

      len = len + brlen - i;
      block->Set(str.c_str() + i, len);
      str.erase(i, len);
      str_len -= len;
      break;
    }
  }
}

void TBBlock::Merge() {
  TBBlock* next_block = GetNext();
  if (next_block && !fragments.GetLast()->IsBreak()) {
    str.append(GetNext()->str);
    str_len = str.size();

    styledit->blocks.Delete(next_block);

    height = 0;  // Ensure that Layout propagate height to remaining blocks.
    Layout(true, true);
  }
}

int32_t TBBlock::CalculateTabWidth(FontFace* font, int32_t xpos) const {
  int tabsize = font->GetStringWidth("x", 1) * TAB_SPACE;
  int p2 = int(xpos / tabsize) * tabsize + tabsize;
  return p2 - xpos;
}

int32_t TBBlock::CalculateStringWidth(FontFace* font, const char* str,
                                      size_t len) const {
  if (styledit->packed.password_on) {
    // Convert the length in number or characters, since that's what matters for
    // password width.
    len = utf8::count_characters(str, len);
    return font->GetStringWidth(special_char_password) * int(len);
  }
  return font->GetStringWidth(str, len);
}

int32_t TBBlock::CalculateLineHeight(FontFace* font) const {
  return font->GetHeight();
}

int32_t TBBlock::CalculateBaseline(FontFace* font) const {
  return font->GetAscent();
}

int TBBlock::GetStartIndentation(FontFace* font, size_t first_line_len) const {
  // Lines beginning with whitespace or list points, should
  // indent to the same as the beginning when wrapped.
  int32_t indentation = 0;
  size_t i = 0;
  while (i < first_line_len) {
    const char* current_str = str.c_str() + i;
    UCS4 uc = utf8::decode_next(str.c_str(), &i, first_line_len);
    switch (uc) {
      case '\t':
        indentation += CalculateTabWidth(font, indentation);
        continue;
      case ' ':
      case '-':
      case '*':
        indentation += CalculateStringWidth(font, current_str, 1);
        continue;
      case 0x2022:  // BULLET
        indentation += CalculateStringWidth(font, current_str, 3);
        continue;
    };
    break;
  }
  return indentation;
}

void TBBlock::Layout(bool update_fragments, bool propagate_height) {
  // Create fragments from the word fragments
  if (update_fragments || !fragments.GetFirst()) {
    Clear();

    int ofs = 0;
    const char* text = str.c_str();
    while (true) {
      int frag_len;
      bool is_embed = false;
      bool more = GetNextFragment(
          &text[ofs],
          styledit->packed.styling_on ? styledit->content_factory : nullptr,
          &frag_len, &is_embed);

      TBTextFragment* fragment = new TBTextFragment();
      if (!fragment) break;

      fragment->Init(this, ofs, frag_len);

      if (is_embed)
        fragment->content = styledit->content_factory->CreateFragmentContent(
            &text[ofs], frag_len);

      fragments.AddLast(fragment);
      ofs += frag_len;

      if (!more) break;
    }
  }

  // Layout

  if (styledit->layout_width <= 0 && styledit->GetSizeAffectsLayout())
    // Don't layout if we have no space. This will happen when setting text
    // before the widget has been layouted. We will relayout when we are
    // resized.
    return;

  int old_line_width_max = line_width_max;
  line_width_max = 0;
  int line_ypos = 0;
  int first_line_indentation = 0;
  TBTextFragment* first_fragment_on_line = fragments.GetFirst();

  while (first_fragment_on_line) {
    int line_width = 0;

    // Get the last fragment that should be laid out on the line while
    // calculating line width and preliminary x positions for the fragments.

    TBTextFragment* last_fragment_on_line = fragments.GetLast();
    if (styledit->packed.wrapping) {
      // If we should wrap, search for the last allowed break point before the
      // overflow.
      TBTextFragment* allowed_last_fragment = nullptr;

      int line_xpos = first_line_indentation;
      for (TBTextFragment* fragment = first_fragment_on_line; fragment;
           fragment = fragment->GetNext()) {
        // Give the fragment the current x. Then tab widths are calculated
        // properly in GetWidth.
        fragment->xpos = line_xpos;
        int fragment_w = fragment->GetWidth(styledit->font);

        // Check if we overflow
        bool overflow = line_xpos + fragment_w > styledit->layout_width;

        if (overflow && allowed_last_fragment) {
          last_fragment_on_line = allowed_last_fragment;
          break;
        }

        // Check if this is a allowed break position
        if (fragment->GetAllowBreakAfter()) {
          if (!fragment->GetNext() ||
              fragment->GetNext()->GetAllowBreakBefore()) {
            allowed_last_fragment = fragment;
            line_width = line_xpos + fragment_w;
          }
        }

        line_xpos += fragment_w;
      }
      if (!allowed_last_fragment) {
        line_width = line_xpos;
      }
    } else {
      // When wrapping is off, just measure and set pos.
      line_width = first_line_indentation;
      for (TBTextFragment* fragment = first_fragment_on_line; fragment;
           fragment = fragment->GetNext()) {
        fragment->xpos = line_width;
        line_width += fragment->GetWidth(styledit->font);
      }
    }

    // Commit line - Layout each fragment on the line.

    int line_height = 0;
    int line_baseline = 0;
    TBTextFragment* fragment = first_fragment_on_line;
    while (fragment) {
      line_height = std::max(fragment->GetHeight(styledit->font), line_height);
      line_baseline =
          std::max(fragment->GetBaseline(styledit->font), line_baseline);

      // These positions are not final. Will be adjusted below.
      fragment->ypos = line_ypos;

      if (fragment == last_fragment_on_line) {
        break;
      }
      fragment = fragment->GetNext();
    }

    // Adjust the position of fragments on the line - now when we know the line
    // totals.
    // x change because of alignment, y change because of fragment baseline vs
    // line baseline.

    int32_t xofs = 0;
    if (TextAlign(align) == TextAlign::kRight) {
      xofs = styledit->layout_width - line_width;
    } else if (TextAlign(align) == TextAlign::kCenter) {
      xofs = (styledit->layout_width - line_width) / 2;
    }

    int adjusted_line_height = line_height;
    fragment = first_fragment_on_line;
    while (fragment) {
      // The fragment need to know these later.
      fragment->line_ypos = line_ypos;
      fragment->line_height = line_height;

      // Adjust the position
      fragment->ypos += line_baseline - fragment->GetBaseline(styledit->font);
      fragment->xpos += xofs;

      // We now know the final position so update content.
      fragment->UpdateContentPos();

      // Total line height may now have changed a bit.
      adjusted_line_height =
          std::max(line_baseline - fragment->GetBaseline(styledit->font) +
                       fragment->GetHeight(styledit->font),
                   adjusted_line_height);

      if (fragment == last_fragment_on_line) break;
      fragment = fragment->GetNext();
    }

    // Update line_height set on fragments if needed
    if (line_height != adjusted_line_height) {
      for (fragment = first_fragment_on_line;
           fragment != last_fragment_on_line->GetNext();
           fragment = fragment->GetNext())
        fragment->line_height = adjusted_line_height;
    }

    line_width_max = std::max(line_width_max, line_width);

    // This was the first line so calculate the indentation to use for the other
    // lines.
    if (styledit->packed.wrapping &&
        first_fragment_on_line == fragments.GetFirst())
      first_line_indentation =
          GetStartIndentation(styledit->font, last_fragment_on_line->ofs +
                                                  last_fragment_on_line->len);

    // Consume line

    line_ypos += adjusted_line_height;

    first_fragment_on_line = last_fragment_on_line->GetNext();
  }

  ypos = GetPrev() ? GetPrev()->ypos + GetPrev()->height : 0;
  SetSize(old_line_width_max, line_width_max, line_ypos, propagate_height);

  Invalidate();
}

void TBBlock::SetSize(int32_t old_w, int32_t new_w, int32_t new_h,
                      bool propagate_height) {
  // Later: could optimize with Scroll here.
  int32_t dh = new_h - height;
  height = new_h;
  if (dh != 0 && propagate_height) {
    TBBlock* block = GetNext();
    while (block) {
      block->ypos = block->GetPrev()->ypos + block->GetPrev()->height;
      block->Invalidate();
      block = block->GetNext();
    }
  }

  // Update content_width and content_height
  // content_width can only be calculated in constant time if we grow larger.
  // If we shrink our width and where equal to content_width, we don't know
  // how wide the widest block is and we set a flag to update it when needed.

  if (!styledit->packed.wrapping && !styledit->packed.multiline_on)
    styledit->content_width = new_w;
  else if (new_w > styledit->content_width)
    styledit->content_width = new_w;
  else if (new_w < old_w && old_w == styledit->content_width)
    styledit->packed.calculate_content_width_needed = 1;

  styledit->content_height =
      styledit->blocks.GetLast()->ypos + styledit->blocks.GetLast()->height;

  if (styledit->listener && styledit->packed.lock_scrollbars_counter == 0 &&
      propagate_height)
    styledit->listener->UpdateScrollbars();
}

TBTextFragment* TBBlock::FindFragment(size_t ofs, bool prefer_first) const {
  TBTextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    if (prefer_first && ofs <= fragment->ofs + fragment->len) return fragment;
    if (!prefer_first && ofs < fragment->ofs + fragment->len) return fragment;
    fragment = fragment->GetNext();
  }
  return fragments.GetLast();
}

TBTextFragment* TBBlock::FindFragment(int32_t x, int32_t y) const {
  TBTextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    if (y < fragment->line_ypos + fragment->line_height) {
      if (x < fragment->xpos + fragment->GetWidth(styledit->font))
        return fragment;
      if (fragment->GetNext() &&
          fragment->GetNext()->line_ypos > fragment->line_ypos)
        return fragment;
    }
    fragment = fragment->GetNext();
  }
  return fragments.GetLast();
}

void TBBlock::Invalidate() {
  if (styledit->listener)
    styledit->listener->Invalidate(
        Rect(0, -styledit->scroll_y + ypos, styledit->layout_width, height));
}

void TBBlock::BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                                   TBTextProps* props, RectRegion& bg_region,
                                   RectRegion& fg_region) {
  if (!styledit->selection.IsBlockSelected(this)) return;

  TBTextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    fragment->BuildSelectionRegion(translate_x, translate_y + ypos, props,
                                   bg_region, fg_region);
    fragment = fragment->GetNext();
  }
}

void TBBlock::Paint(int32_t translate_x, int32_t translate_y,
                    TBTextProps* props) {
  TMPDEBUG(styledit->listener->DrawRect(
      Rect(translate_x, translate_y + ypos, styledit->layout_width, height),
      Color(255, 200, 0, 128)));

  TBTextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    fragment->Paint(translate_x, translate_y + ypos, props);
    fragment = fragment->GetNext();
  }
}

TBTextFragment::~TBTextFragment() { delete content; }

void TBTextFragment::Init(TBBlock* block, uint16_t ofs, uint16_t len) {
  this->block = block;
  this->ofs = ofs;
  this->len = len;
}

void TBTextFragment::UpdateContentPos() {
  if (content) content->UpdatePos(xpos, ypos + block->ypos);
}

void TBTextFragment::BuildSelectionRegion(int32_t translate_x,
                                          int32_t translate_y,
                                          TBTextProps* props,
                                          RectRegion& bg_region,
                                          RectRegion& fg_region) {
  if (!block->styledit->selection.IsFragmentSelected(this)) return;

  int x = translate_x + xpos;
  int y = translate_y + ypos;
  FontFace* font = props->GetFont();

  if (content) {
    // Selected embedded content should add to the foreground region.
    fg_region.IncludeRect(Rect(x, y, GetWidth(font), GetHeight(font)));
    return;
  }

  // Selected text should add to the backgroud region.
  TBSelection* sel = &block->styledit->selection;

  size_t sofs1 = sel->start.block == block ? sel->start.ofs : 0;
  size_t sofs2 = sel->stop.block == block ? sel->stop.ofs : block->str_len;
  sofs1 = std::max(sofs1, size_t(ofs));
  sofs2 = std::min(sofs2, size_t(ofs + len));

  int s1x = GetStringWidth(font, block->str.c_str() + ofs, sofs1 - ofs);
  int s2x = GetStringWidth(font, block->str.c_str() + sofs1, sofs2 - sofs1);

  bg_region.IncludeRect(Rect(x + s1x, y, s2x, GetHeight(font)));
}

void TBTextFragment::Paint(int32_t translate_x, int32_t translate_y,
                           TBTextProps* props) {
  TBStyleEditListener* listener = block->styledit->listener;

  int x = translate_x + xpos;
  int y = translate_y + ypos;
  Color color = props->data->text_color;
  FontFace* font = props->GetFont();

  if (content) {
    content->Paint(this, translate_x, translate_y, props);
    return;
  }
  TMPDEBUG(listener->DrawRect(Rect(x, y, GetWidth(font), GetHeight(font)),
                              Color(255, 255, 255, 128)));

  if (IsBreak()) {
    listener->OnBreak();
  }
  if (block->styledit->packed.password_on) {
    int cw = block->CalculateStringWidth(font, special_char_password);
    size_t num_char = utf8::count_characters(Str(), len);
    for (size_t i = 0; i < num_char; i++)
      listener->DrawString(int(x + i * cw), y, font, color,
                           special_char_password);
  } else if (block->styledit->packed.show_whitespace) {
    if (IsTab())
      listener->DrawString(x, y, font, color, special_char_tab);
    else if (IsBreak())
      listener->DrawString(x, y, font, color, special_char_newln);
    else if (IsSpace())
      listener->DrawString(x, y, font, color, special_char_space);
    else
      listener->DrawString(x, y, font, color, Str(), len);
  } else if (!IsTab() && !IsBreak() && !IsSpace())
    listener->DrawString(x, y, font, color, Str(), len);

  if (props->data->underline) {
    int line_h = font->GetHeight() / 16;
    line_h = std::max(line_h, 1);
    listener->DrawRectFill(
        Rect(x, y + GetBaseline(font) + 1, GetWidth(font), line_h), color);
  }
}

void TBTextFragment::Click(int button, ModifierKeys modifierkeys) {
  if (content) content->Click(this, button, modifierkeys);
}

int32_t TBTextFragment::GetWidth(FontFace* font) {
  if (content) return content->GetWidth(font, this);
  if (IsBreak()) return 0;
  if (IsTab()) return block->CalculateTabWidth(font, xpos);
  return block->CalculateStringWidth(font, block->str.c_str() + ofs, len);
}

int32_t TBTextFragment::GetHeight(FontFace* font) {
  if (content) return content->GetHeight(font, this);
  return block->CalculateLineHeight(font);
}

int32_t TBTextFragment::GetBaseline(FontFace* font) {
  if (content) return content->GetBaseline(font, this);
  return block->CalculateBaseline(font);
}

int32_t TBTextFragment::GetCharX(FontFace* font, size_t ofs) {
  assert(ofs >= 0 && ofs <= len);

  if (IsEmbedded() || IsTab()) return ofs == 0 ? 0 : GetWidth(font);
  if (IsBreak()) return 0;

  return block->CalculateStringWidth(font, block->str.c_str() + this->ofs, ofs);
}

size_t TBTextFragment::GetCharOfs(FontFace* font, int32_t x) {
  if (IsEmbedded() || IsTab()) return x > GetWidth(font) / 2 ? 1 : 0;
  if (IsBreak()) return 0;

  const char* str = block->str.c_str() + ofs;
  size_t i = 0;
  while (i < len) {
    size_t pos = i;
    utf8::move_inc(str, &i, len);
    size_t last_char_len = i - pos;
    // Always measure from the beginning of the fragment because of eventual
    // kerning & text shaping etc.
    int width_except_last_char =
        block->CalculateStringWidth(font, str, i - last_char_len);
    int width = block->CalculateStringWidth(font, str, i);
    if (x < width - (width - width_except_last_char) / 2) return pos;
  }
  return len;
}

int32_t TBTextFragment::GetStringWidth(FontFace* font, const char* str,
                                       size_t len) {
  if (IsTab()) return len == 0 ? 0 : block->CalculateTabWidth(font, xpos);
  if (IsBreak()) return len == 0 ? 0 : 8;
  return block->CalculateStringWidth(font, str, len);
}

bool TBTextFragment::IsBreak() const {
  return Str()[0] == '\r' || Str()[0] == '\n';
}

bool TBTextFragment::IsSpace() const { return is_space(Str()[0]); }

bool TBTextFragment::IsTab() const { return Str()[0] == '\t'; }

bool TBTextFragment::GetAllowBreakBefore() const {
  if (content) return content->GetAllowBreakBefore();
  if (len && !is_never_break_before(block->str.c_str(), ofs)) return true;
  return false;
}

bool TBTextFragment::GetAllowBreakAfter() const {
  if (content) return content->GetAllowBreakAfter();
  if (len && !is_never_break_after(block->str.c_str(), ofs + len - 1))
    return true;
  return false;
}

TBStyleEdit::TBStyleEdit()
    : listener(nullptr),
      content_factory(&default_content_factory),
      layout_width(0),
      layout_height(0),
      content_width(0),
      content_height(0),
      caret(nullptr),
      selection(nullptr),
      scroll_x(0),
      scroll_y(0),
      select_state(0),
      mousedown_fragment(nullptr),
      font(nullptr),
      align(TextAlign::kLeft),
      packed_init(0) {
  caret.styledit = this;
  selection.styledit = this;
  TMPDEBUG(packed.show_whitespace = true);

  font_desc = g_font_manager->GetDefaultFontDescription();
  font = g_font_manager->GetFontFace(font_desc);

#ifdef TB_TARGET_WINDOWS
  packed.win_style_br = 1;
#endif
  packed.selection_on = 1;

  Clear();
}

TBStyleEdit::~TBStyleEdit() {
  listener->CaretBlinkStop();
  Clear(false);
}

void TBStyleEdit::SetListener(TBStyleEditListener* listener) {
  this->listener = listener;
}

void TBStyleEdit::SetContentFactory(
    TBTextFragmentContentFactory* content_factory) {
  if (content_factory)
    this->content_factory = content_factory;
  else
    this->content_factory = &default_content_factory;
}

void TBStyleEdit::SetFont(const FontDescription& font_desc) {
  if (this->font_desc == font_desc) return;
  this->font_desc = font_desc;
  font = g_font_manager->GetFontFace(font_desc);
  Reformat(true);
}

void TBStyleEdit::Clear(bool init_new) {
  undoredo.Clear(true, true);
  selection.SelectNothing();

  if (init_new && blocks.GetFirst() && IsEmpty()) return;

  for (TBBlock* block = blocks.GetFirst(); block; block = block->GetNext())
    block->Invalidate();
  blocks.DeleteAll();

  if (init_new) {
    blocks.AddLast(new TBBlock(this));
    blocks.GetFirst()->Set("", 0);
  }

  caret.Place(blocks.GetFirst(), 0);
  caret.UpdateWantedX();
}

void TBStyleEdit::ScrollIfNeeded(bool x, bool y) {
  if (layout_width <= 0 || layout_height <= 0)
    return;  // This is likely during construction before layout.

  int32_t newx = scroll_x, newy = scroll_y;
  if (x) {
    if (caret.x - scroll_x < 0) newx = caret.x;
    if (caret.x + caret.width - scroll_x > layout_width)
      newx = caret.x + caret.width - layout_width;
  }
  if (y) {
    if (caret.y - scroll_y < 0) newy = caret.y;
    if (caret.y + caret.height - scroll_y > layout_height)
      newy = caret.y + caret.height - layout_height;
  }
  SetScrollPos(newx, newy);
}

void TBStyleEdit::SetScrollPos(int32_t x, int32_t y) {
  x = std::min(x, GetContentWidth() - layout_width);
  y = std::min(y, GetContentHeight() - layout_height);
  x = std::max(x, 0);
  y = std::max(y, 0);
  if (!packed.multiline_on) y = 0;
  int dx = scroll_x - x;
  int dy = scroll_y - y;
  if (dx || dy) {
    scroll_x = x;
    scroll_y = y;
    listener->Scroll(dx, dy);
  }
}

void TBStyleEdit::BeginLockScrollbars() { packed.lock_scrollbars_counter++; }

void TBStyleEdit::EndLockScrollbars() {
  packed.lock_scrollbars_counter--;
  if (listener && packed.lock_scrollbars_counter == 0)
    listener->UpdateScrollbars();
}

void TBStyleEdit::SetLayoutSize(int32_t width, int32_t height,
                                bool is_virtual_reformat) {
  if (width == layout_width && height == layout_height) return;

  bool reformat = layout_width != width;
  layout_width = width;
  layout_height = height;

  if (reformat && GetSizeAffectsLayout()) Reformat(false);

  caret.UpdatePos();
  caret.UpdateWantedX();

  if (!is_virtual_reformat)
    SetScrollPos(scroll_x,
                 scroll_y);  ///< Trig a bounds check (scroll if outside)
}

bool TBStyleEdit::GetSizeAffectsLayout() const {
  if (packed.wrapping || align != TextAlign::kLeft) return true;
  return false;
}

void TBStyleEdit::Reformat(bool update_fragments) {
  int ypos = 0;
  BeginLockScrollbars();
  TBBlock* block = blocks.GetFirst();
  while (block) {
    // Update ypos directly instead of using "propagate_height" since
    // propagating
    // would iterate forward through all remaining blocks and we're going to
    // visit
    // them all anyway.
    block->ypos = ypos;
    block->Layout(update_fragments, false);
    ypos += block->height;
    block = block->GetNext();
  }
  EndLockScrollbars();
  listener->Invalidate(Rect(0, 0, layout_width, layout_height));
}

int32_t TBStyleEdit::GetContentWidth() {
  if (packed.calculate_content_width_needed) {
    packed.calculate_content_width_needed = 0;
    content_width = 0;
    TBBlock* block = blocks.GetFirst();
    while (block) {
      content_width = std::max(content_width, block->line_width_max);
      block = block->GetNext();
    }
  }
  return content_width;
}

int32_t TBStyleEdit::GetContentHeight() const { return content_height; }

void TBStyleEdit::Paint(const Rect& rect, const FontDescription& font_desc,
                        const Color& text_color) {
  TBTextProps props(font_desc, text_color);

  // Find the first visible block
  TBBlock* first_visible_block = blocks.GetFirst();
  while (first_visible_block) {
    if (first_visible_block->ypos + first_visible_block->height - scroll_y >= 0)
      break;
    first_visible_block = first_visible_block->GetNext();
  }

  // Get the selection region for all visible blocks
  RectRegion bg_region, fg_region;
  if (selection.IsSelected()) {
    TBBlock* block = first_visible_block;
    while (block) {
      if (block->ypos - scroll_y > rect.y + rect.h) break;
      block->BuildSelectionRegion(-scroll_x, -scroll_y, &props, bg_region,
                                  fg_region);
      block = block->GetNext();
    }

    // Paint bg selection
    for (int i = 0; i < bg_region.GetNumRects(); i++)
      listener->DrawTextSelectionBg(bg_region.GetRect(i));
  }

  // Paint the content
  TBBlock* block = first_visible_block;
  while (block) {
    if (block->ypos - scroll_y > rect.y + rect.h) break;
    block->Paint(-scroll_x, -scroll_y, &props);
    block = block->GetNext();
  }

  // Paint fg selection
  for (int i = 0; i < fg_region.GetNumRects(); i++)
    listener->DrawTextSelectionBg(fg_region.GetRect(i));

  // Paint caret
  caret.Paint(-scroll_x, -scroll_y);
}

void TBStyleEdit::InsertBreak() {
  if (!packed.multiline_on) return;

  const char* new_line_str = packed.win_style_br ? "\r\n" : "\n";

  // If we stand at the end and don't have any ending break, we're standing at
  // the last line and
  // should insert breaks twice. One to end the current line, and one for the
  // new empty line.
  if (caret.pos.ofs == caret.pos.block->str_len &&
      !caret.pos.block->fragments.GetLast()->IsBreak())
    new_line_str = packed.win_style_br ? "\r\n\r\n" : "\n\n";

  InsertText(new_line_str);

  caret.AvoidLineBreak();
  if (caret.pos.block->GetNext()) caret.Place(caret.pos.block->GetNext(), 0);
}

void TBStyleEdit::InsertText(const char* text, size_t len, bool after_last,
                             bool clear_undo_redo) {
  if (len == std::string::npos) len = strlen(text);
  assert(len < 0x777777);

  selection.RemoveContent();

  if (after_last)
    caret.Place(blocks.GetLast(), blocks.GetLast()->str_len, false);

  size_t len_inserted =
      caret.pos.block->InsertText(caret.pos.ofs, text, len, true);
  if (clear_undo_redo)
    undoredo.Clear(true, true);
  else
    undoredo.Commit(this, caret.GetGlobalOfs(), len_inserted, text, true);

  caret.Place(caret.pos.block, caret.pos.ofs + len, false);
  caret.UpdatePos();
  caret.UpdateWantedX();
}

TBBlock* TBStyleEdit::FindBlock(int32_t y) const {
  TBBlock* block = blocks.GetFirst();
  while (block) {
    if (y < block->ypos + block->height) return block;
    block = block->GetNext();
  }
  return blocks.GetLast();
}

bool TBStyleEdit::KeyDown(int key, SpecialKey special_key,
                          ModifierKeys modifierkeys) {
  if (select_state) return false;

  bool handled = true;
  bool move_caret =
      special_key == SpecialKey::kLeft || special_key == SpecialKey::kRight ||
      special_key == SpecialKey::kUp || special_key == SpecialKey::kDown ||
      special_key == SpecialKey::kHome || special_key == SpecialKey::kEnd ||
      special_key == SpecialKey::kPageUp ||
      special_key == SpecialKey::kPageDown;

  if (!any(modifierkeys & ModifierKeys::kShift) && move_caret) {
    selection.SelectNothing();
  }

  TBTextOfs old_caret_pos = caret.pos;
  TBTextFragment* old_caret_elm = caret.GetFragment();

  if ((special_key == SpecialKey::kUp || special_key == SpecialKey::kDown) &&
      any(modifierkeys & ModifierKeys::kCtrl)) {
    int32_t line_height = old_caret_pos.block->CalculateLineHeight(font);
    int32_t new_y = scroll_y + (special_key == SpecialKey::kUp ? -line_height
                                                               : line_height);
    SetScrollPos(scroll_x, new_y);
  } else if (special_key == SpecialKey::kLeft) {
    caret.Move(false, any(modifierkeys & ModifierKeys::kCtrl));
  } else if (special_key == SpecialKey::kRight) {
    caret.Move(true, any(modifierkeys & ModifierKeys::kCtrl));
  } else if (special_key == SpecialKey::kUp) {
    handled =
        caret.Place(Point(caret.wanted_x, old_caret_pos.block->ypos +
                                              old_caret_elm->line_ypos - 1));
  } else if (special_key == SpecialKey::kDown) {
    handled = caret.Place(Point(
        caret.wanted_x, old_caret_pos.block->ypos + old_caret_elm->line_ypos +
                            old_caret_elm->line_height + 1));
  } else if (special_key == SpecialKey::kPageUp) {
    caret.Place(Point(caret.wanted_x, caret.y - layout_height));
  } else if (special_key == SpecialKey::kPageDown) {
    caret.Place(Point(caret.wanted_x,
                      caret.y + layout_height + old_caret_elm->line_height));
  } else if (special_key == SpecialKey::kHome &&
             any(modifierkeys & ModifierKeys::kCtrl)) {
    caret.Place(Point(0, 0));
  } else if (special_key == SpecialKey::kEnd &&
             any(modifierkeys & ModifierKeys::kCtrl)) {
    caret.Place(
        Point(32000, blocks.GetLast()->ypos + blocks.GetLast()->height));
  } else if (special_key == SpecialKey::kHome) {
    caret.Place(Point(0, caret.y));
  } else if (special_key == SpecialKey::kEnd) {
    caret.Place(Point(32000, caret.y));
  } else if (key == '8' && any(modifierkeys & ModifierKeys::kCtrl)) {
    packed.show_whitespace = !packed.show_whitespace;
    listener->Invalidate(Rect(0, 0, layout_width, layout_height));
  } else if (!packed.read_only && (special_key == SpecialKey::kDelete ||
                                   special_key == SpecialKey::kBackspace)) {
    if (!selection.IsSelected()) {
      caret.Move(special_key == SpecialKey::kDelete,
                 any(modifierkeys & ModifierKeys::kCtrl));
      selection.SelectToCaret(old_caret_pos.block, old_caret_pos.ofs);
    }
    selection.RemoveContent();
  } else if (!packed.read_only && !any(modifierkeys & ModifierKeys::kShift) &&
             (special_key == SpecialKey::kTab && packed.multiline_on)) {
    InsertText("\t", 1);
  } else if (!packed.read_only &&
             (special_key == SpecialKey::kEnter && packed.multiline_on) &&
             !any(modifierkeys & ModifierKeys::kCtrl)) {
    InsertBreak();
  } else if (!packed.read_only &&
             (key && !any(modifierkeys & ModifierKeys::kCtrl)) &&
             special_key != SpecialKey::kEnter) {
    char utf8[8];
    int len = utf8::encode(key, utf8);
    InsertText(utf8, len);
  } else {
    handled = false;
  }

  if (any(modifierkeys & ModifierKeys::kShift) && move_caret) {
    selection.SelectToCaret(old_caret_pos.block, old_caret_pos.ofs);
  }

  if (!(special_key == SpecialKey::kUp || special_key == SpecialKey::kDown ||
        special_key == SpecialKey::kPageUp ||
        special_key == SpecialKey::kPageDown)) {
    caret.UpdateWantedX();
  }

  caret.ResetBlink();

  // Hooks
  if (!move_caret && handled) {
    listener->OnChange();
  }
  if (special_key == SpecialKey::kEnter &&
      !any(modifierkeys & ModifierKeys::kCtrl)) {
    if (listener->OnEnter()) {
      handled = true;
    }
  }
  if (handled) ScrollIfNeeded();

  return handled;
}

void TBStyleEdit::Cut() {
  if (packed.password_on) return;
  Copy();
  KeyDown(0, SpecialKey::kDelete, ModifierKeys::kNone);
}

void TBStyleEdit::Copy() {
  if (packed.password_on) return;
  selection.CopyToClipboard();
}

void TBStyleEdit::Paste() {
  if (TBClipboard::HasText()) {
    auto text = TBClipboard::GetText();
    InsertText(text, text.size());
    ScrollIfNeeded(true, true);
    listener->OnChange();
  }
}

void TBStyleEdit::Delete() {
  if (selection.IsSelected()) {
    selection.RemoveContent();
    listener->OnChange();
  }
}

void TBStyleEdit::Undo() {
  if (CanUndo()) {
    undoredo.Undo(this);
    listener->OnChange();
  }
}

void TBStyleEdit::Redo() {
  if (CanRedo()) {
    undoredo.Redo(this);
    listener->OnChange();
  }
}

bool TBStyleEdit::MouseDown(const Point& point, int button, int clicks,
                            ModifierKeys modifierkeys, bool touch) {
  if (button != 1) return false;

  if (touch) {
    mousedown_point = Point(point.x + scroll_x, point.y + scroll_y);
  } else if (packed.selection_on) {
    // if (modifierkeys & P_SHIFT) // Select to new caretpos
    //{
    //}
    // else // Start selection
    {
      mousedown_point = Point(point.x + scroll_x, point.y + scroll_y);
      selection.SelectNothing();

      // clicks is 1 to infinite, and here we support only doubleclick, so make
      // it either single or double.
      select_state = ((clicks - 1) % 2) + 1;

      MouseMove(point);

      if (caret.pos.block)
        mousedown_fragment = caret.pos.block->FindFragment(
            mousedown_point.x, mousedown_point.y - caret.pos.block->ypos);
    }
    caret.ResetBlink();
  }
  return true;
}

bool TBStyleEdit::MouseUp(const Point& point, int button,
                          ModifierKeys modifierkeys, bool touch) {
  if (button != 1) return false;

  if (touch && !TBWidget::cancel_click) {
    selection.SelectNothing();
    caret.Place(mousedown_point);
    caret.UpdateWantedX();
    caret.ResetBlink();
  }

  select_state = 0;
  if (caret.pos.block && !TBWidget::cancel_click) {
    TBTextFragment* fragment = caret.pos.block->FindFragment(
        point.x + scroll_x, point.y + scroll_y - caret.pos.block->ypos);
    if (fragment && fragment == mousedown_fragment)
      fragment->Click(button, modifierkeys);
  }
  return true;
}

bool TBStyleEdit::MouseMove(const Point& point) {
  if (select_state) {
    Point p(point.x + scroll_x, point.y + scroll_y);
    selection.Select(mousedown_point, p);

    if (select_state == 2) {
      bool has_initial_selection = selection.IsSelected();

      if (has_initial_selection)
        caret.Place(selection.start.block, selection.start.ofs);
      caret.Move(false, true);
      selection.start.Set(caret.pos);

      if (has_initial_selection)
        caret.Place(selection.stop.block, selection.stop.ofs);
      caret.Move(true, true);
      selection.stop.Set(caret.pos);

      selection.CorrectOrder();
      caret.UpdateWantedX();
    }
    return true;
  }
  return false;
}

void TBStyleEdit::Focus(bool focus) {
  if (focus)
    listener->CaretBlinkStart();
  else
    listener->CaretBlinkStop();

  caret.on = focus;
  caret.Invalidate();
  selection.Invalidate();
}

void TBStyleEdit::SetText(const char* text, CaretPosition pos) {
  SetText(text, strlen(text), pos);
}

void TBStyleEdit::SetText(const char* text, size_t text_len,
                          CaretPosition pos) {
  if (!text || !*text) {
    Clear(true);
    caret.UpdateWantedX();
    ScrollIfNeeded(true, true);
    return;
  }

  Clear(true);
  blocks.GetFirst()->InsertText(0, text, text_len, true);

  caret.Place(blocks.GetFirst(), 0);
  caret.UpdateWantedX();
  ScrollIfNeeded(true, false);

  if (pos == CaretPosition::kEnd) {
    caret.Place(blocks.GetLast(), blocks.GetLast()->str_len);
  }

  listener->OnChange();
}

bool TBStyleEdit::Load(const char* filename) {
  TBFile* f = TBFile::Open(filename, TBFile::Mode::kRead);
  if (!f) return false;
  size_t num_bytes = f->Size();

  char* str = new char[num_bytes + 1];
  if (!str) {
    delete f;
    return false;
  }

  num_bytes = f->Read(str, 1, num_bytes);
  str[num_bytes] = 0;

  delete f;

  SetText(str);

  delete[] str;
  return true;
}

std::string TBStyleEdit::GetText() {
  TBSelection tmp_selection(this);
  tmp_selection.SelectAll();
  return tmp_selection.GetText();
}

bool TBStyleEdit::IsEmpty() const {
  return blocks.GetFirst() == blocks.GetLast() &&
         blocks.GetFirst()->str.empty();
}

void TBStyleEdit::SetAlign(TextAlign align) {
  this->align = align;
  // Call SetAlign on all blocks currently selected, or the block of the current
  // caret position.
  TBBlock* start =
      selection.IsSelected() ? selection.start.block : caret.pos.block;
  TBBlock* stop =
      selection.IsSelected() ? selection.stop.block : caret.pos.block;
  while (start && start != stop->GetNext()) {
    start->SetAlign(align);
    start = start->GetNext();
  }
}

void TBStyleEdit::SetMultiline(bool multiline) {
  packed.multiline_on = multiline;
}

void TBStyleEdit::SetStyling(bool styling) { packed.styling_on = styling; }

void TBStyleEdit::SetReadOnly(bool readonly) { packed.read_only = readonly; }

void TBStyleEdit::SetSelection(bool selection) {
  packed.selection_on = selection;
}

void TBStyleEdit::SetPassword(bool password) {
  if (packed.password_on == password) return;
  packed.password_on = password;
  Reformat(true);
}

void TBStyleEdit::SetWrapping(bool wrapping) {
  if (packed.wrapping == wrapping) return;
  packed.wrapping = wrapping;
  Reformat(false);
}

TBUndoRedoStack::~TBUndoRedoStack() { Clear(true, true); }

void TBUndoRedoStack::Undo(TBStyleEdit* styledit) {
  if (!undos.GetNumItems()) return;
  TBUndoEvent* e = undos.Remove(undos.GetNumItems() - 1);
  redos.Add(e);
  Apply(styledit, e, true);
}

void TBUndoRedoStack::Redo(TBStyleEdit* styledit) {
  if (!redos.GetNumItems()) return;
  TBUndoEvent* e = redos.Remove(redos.GetNumItems() - 1);
  undos.Add(e);
  Apply(styledit, e, false);
}

void TBUndoRedoStack::Apply(TBStyleEdit* styledit, TBUndoEvent* e,
                            bool reverse) {
  applying = true;
  if (e->insert == reverse) {
    styledit->selection.SelectNothing();
    styledit->caret.SetGlobalOfs(e->gofs, false);
    assert(TBTextOfs(styledit->caret.pos).GetGlobalOfs(styledit) == e->gofs);

    TBTextOfs start = styledit->caret.pos;
    styledit->caret.SetGlobalOfs(e->gofs + e->text.size(), false);
    assert(TBTextOfs(styledit->caret.pos).GetGlobalOfs(styledit) ==
           e->gofs + e->text.size());

    styledit->selection.Select(start, styledit->caret.pos);
    styledit->selection.RemoveContent();
  } else {
    styledit->selection.SelectNothing();
    styledit->caret.SetGlobalOfs(e->gofs, true, true);
    styledit->InsertText(e->text);
    size_t text_len = e->text.size();
    if (text_len > 1) styledit->selection.Select(e->gofs, e->gofs + text_len);
  }
  styledit->ScrollIfNeeded(true, true);
  applying = false;
}

void TBUndoRedoStack::Clear(bool clear_undo, bool clear_redo) {
  assert(!applying);
  if (clear_undo) undos.DeleteAll();
  if (clear_redo) redos.DeleteAll();
}

TBUndoEvent* TBUndoRedoStack::Commit(TBStyleEdit* styledit, size_t gofs,
                                     size_t len, const char* text,
                                     bool insert) {
  if (applying || styledit->packed.read_only) return nullptr;
  Clear(false, true);

  // If we're inserting a single character, check if we want to append it to the
  // previous event.
  if (insert && undos.GetNumItems()) {
    size_t num_char = utf8::count_characters(text, len);
    TBUndoEvent* e = undos[undos.GetNumItems() - 1];
    if (num_char == 1 && e->insert && e->gofs + e->text.size() == gofs) {
      // Appending a space to other space(s) should append
      if ((text[0] == ' ' && !strpbrk(e->text.c_str(), "\r\n")) ||
          // But non spaces should not
          !strpbrk(e->text.c_str(), " \r\n")) {
        e->text.append(text, len);
        return e;
      }
    }
  }

  // Create a new event
  if (TBUndoEvent* e = new TBUndoEvent()) {
    e->gofs = gofs;
    e->text.assign(text, len);
    e->insert = insert;
    undos.Add(e);
    return e;
  }

  // OOM
  Clear(true, true);
  return nullptr;
}

}  // namespace tb
