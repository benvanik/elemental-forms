/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/text/font_manager.h"
#include "el/text/text_view.h"
#include "el/text/utf8.h"
#include "el/util/clipboard.h"
#include "el/util/file.h"
#include "el/util/rect_region.h"
#include "el/util/string.h"
#include "el/util/string_builder.h"

namespace el {
namespace text {

// TODO(benvanik): make a runtime per-textbox option.
const bool kShowWhitespace = false;

TextView::TextView() {
  caret.style_edit = this;
  selection.style_edit = this;
  packed.show_whitespace = kShowWhitespace;

  font_desc = el::text::FontManager::get()->default_font_description();
  font = el::text::FontManager::get()->GetFontFace(font_desc);

// TODO(benvanik): make a runtime per-textbox option.
#if WIN32
  packed.win_style_br = 1;
#endif  // WIN32
  packed.selection_on = 1;

  Clear();
}

TextView::~TextView() {
  listener->CaretBlinkStop();
  Clear(false);
}

void TextView::SetListener(TextViewListener* listener) {
  this->listener = listener;
}

void TextView::SetContentFactory(TextFragmentContentFactory* content_factory) {
  if (content_factory) {
    this->content_factory = content_factory;
  } else {
    this->content_factory = &default_content_factory;
  }
}

void TextView::SetFont(const FontDescription& font_desc) {
  if (this->font_desc == font_desc) return;
  this->font_desc = font_desc;
  font = el::text::FontManager::get()->GetFontFace(font_desc);
  Reformat(true);
}

void TextView::Clear(bool init_new) {
  undo_stack.Clear(true, true);
  selection.SelectNothing();

  if (init_new && blocks.GetFirst() && empty()) {
    return;
  }

  for (TextBlock* block = blocks.GetFirst(); block; block = block->GetNext()) {
    block->Invalidate();
  }
  blocks.DeleteAll();

  if (init_new) {
    blocks.AddLast(new TextBlock(this));
    blocks.GetFirst()->Set("", 0);
  }

  caret.Place(blocks.GetFirst(), 0);
  caret.UpdateWantedX();
}

void TextView::ScrollIfNeeded(bool x, bool y) {
  if (layout_width <= 0 || layout_height <= 0) {
    return;  // This is likely during construction before layout.
  }

  int32_t newx = scroll_x, newy = scroll_y;
  if (x) {
    if (caret.x - scroll_x < 0) {
      newx = caret.x;
    }
    if (caret.x + caret.width - scroll_x > layout_width) {
      newx = caret.x + caret.width - layout_width;
    }
  }
  if (y) {
    if (caret.y - scroll_y < 0) {
      newy = caret.y;
    }
    if (caret.y + caret.height - scroll_y > layout_height) {
      newy = caret.y + caret.height - layout_height;
    }
  }
  SetScrollPos(newx, newy);
}

void TextView::SetScrollPos(int32_t x, int32_t y) {
  x = std::min(x, GetContentWidth() - layout_width);
  y = std::min(y, GetContentHeight() - layout_height);
  x = std::max(x, 0);
  y = std::max(y, 0);
  if (!packed.multiline_on) {
    y = 0;
  }
  int dx = scroll_x - x;
  int dy = scroll_y - y;
  if (dx || dy) {
    scroll_x = x;
    scroll_y = y;
    listener->Scroll(dx, dy);
  }
}

void TextView::BeginLockScrollbars() { packed.lock_scrollbars_counter++; }

void TextView::EndLockScrollbars() {
  packed.lock_scrollbars_counter--;
  if (listener && packed.lock_scrollbars_counter == 0) {
    listener->UpdateScrollbars();
  }
}

void TextView::SetLayoutSize(int32_t width, int32_t height,
                             bool is_virtual_reformat) {
  if (width == layout_width && height == layout_height) return;

  bool reformat = layout_width != width;
  layout_width = width;
  layout_height = height;

  if (reformat && GetSizeAffectsLayout()) {
    Reformat(false);
  }

  caret.UpdatePos();
  caret.UpdateWantedX();

  if (!is_virtual_reformat) {
    // Trigger a bounds check (scroll if outside).
    SetScrollPos(scroll_x, scroll_y);
  }
}

bool TextView::GetSizeAffectsLayout() const {
  if (packed.wrapping || align != TextAlign::kLeft) {
    return true;
  }
  return false;
}

void TextView::Reformat(bool update_fragments) {
  int ypos = 0;
  BeginLockScrollbars();
  TextBlock* block = blocks.GetFirst();
  while (block) {
    // Update ypos directly instead of using "propagate_height" since
    // propagating would iterate forward through all remaining blocks and we're
    // going to visit them all anyway.
    block->ypos = ypos;
    block->Layout(update_fragments, false);
    ypos += block->height;
    block = block->GetNext();
  }
  EndLockScrollbars();
  listener->Invalidate(Rect(0, 0, layout_width, layout_height));
}

int32_t TextView::GetContentWidth() {
  if (packed.calculate_content_width_needed) {
    packed.calculate_content_width_needed = 0;
    content_width = 0;
    TextBlock* block = blocks.GetFirst();
    while (block) {
      content_width = std::max(content_width, block->line_width_max);
      block = block->GetNext();
    }
  }
  return content_width;
}

int32_t TextView::GetContentHeight() const { return content_height; }

void TextView::Paint(const Rect& rect, const FontDescription& font_desc,
                     const Color& text_color) {
  TextProps props(font_desc, text_color);

  // Find the first visible block.
  TextBlock* first_visible_block = blocks.GetFirst();
  while (first_visible_block) {
    if (first_visible_block->ypos + first_visible_block->height - scroll_y >=
        0) {
      break;
    }
    first_visible_block = first_visible_block->GetNext();
  }

  // Get the selection region for all visible blocks.
  util::RectRegion bg_region;
  util::RectRegion fg_region;
  if (selection.IsSelected()) {
    TextBlock* block = first_visible_block;
    while (block) {
      if (block->ypos - scroll_y > rect.y + rect.h) {
        break;
      }
      block->BuildSelectionRegion(-scroll_x, -scroll_y, &props, &bg_region,
                                  &fg_region);
      block = block->GetNext();
    }

    // Paint bg selection.
    for (int i = 0; i < bg_region.size(); ++i) {
      listener->DrawTextSelectionBg(bg_region[i]);
    }
  }

  // Paint the content.
  TextBlock* block = first_visible_block;
  while (block) {
    if (block->ypos - scroll_y > rect.y + rect.h) {
      break;
    }
    block->Paint(-scroll_x, -scroll_y, &props);
    block = block->GetNext();
  }

  // Paint fg selection.
  for (int i = 0; i < fg_region.size(); ++i) {
    listener->DrawTextSelectionBg(fg_region[i]);
  }

  // Paint caret.
  caret.Paint(-scroll_x, -scroll_y);
}

void TextView::InsertBreak() {
  if (!packed.multiline_on) {
    return;
  }

  const char* new_line_str = packed.win_style_br ? "\r\n" : "\n";

  // If we stand at the end and don't have any ending break, we're standing at
  // the last line and should insert breaks twice. One to end the current line,
  // and one for the new empty line.
  if (caret.pos.ofs == caret.pos.block->str_len &&
      !caret.pos.block->fragments.GetLast()->IsBreak()) {
    new_line_str = packed.win_style_br ? "\r\n\r\n" : "\n\n";
  }

  InsertText(new_line_str);

  caret.AvoidLineBreak();
  if (caret.pos.block->GetNext()) {
    caret.Place(caret.pos.block->GetNext(), 0);
  }
}

void TextView::InsertText(const char* text, size_t len, bool after_last,
                          bool clear_undo_redo) {
  if (len == std::string::npos) {
    len = strlen(text);
  }
  assert(len < 0x777777);

  selection.RemoveContent();

  if (after_last) {
    caret.Place(blocks.GetLast(), blocks.GetLast()->str_len, false);
  }

  size_t len_inserted =
      caret.pos.block->InsertText(caret.pos.ofs, text, len, true);
  if (clear_undo_redo) {
    undo_stack.Clear(true, true);
  } else {
    undo_stack.Commit(this, caret.global_offset(), len_inserted, text, true);
  }

  caret.Place(caret.pos.block, caret.pos.ofs + len, false);
  caret.UpdatePos();
  caret.UpdateWantedX();
}

TextBlock* TextView::FindBlock(int32_t y) const {
  TextBlock* block = blocks.GetFirst();
  while (block) {
    if (y < block->ypos + block->height) {
      return block;
    }
    block = block->GetNext();
  }
  return blocks.GetLast();
}

bool TextView::KeyDown(int key, SpecialKey special_key,
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

  TextOffset old_caret_pos = caret.pos;
  TextFragment* old_caret_elm = caret.fragment();

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

  // Hooks.
  if (!move_caret && handled) {
    listener->OnChange();
  }
  if (special_key == SpecialKey::kEnter &&
      !any(modifierkeys & ModifierKeys::kCtrl)) {
    if (listener->OnEnter()) {
      handled = true;
    }
  }
  if (handled) {
    ScrollIfNeeded();
  }

  return handled;
}

void TextView::Cut() {
  if (packed.password_on) return;
  Copy();
  KeyDown(0, SpecialKey::kDelete, ModifierKeys::kNone);
}

void TextView::Copy() {
  if (packed.password_on) return;
  selection.CopyToClipboard();
}

void TextView::Paste() {
  if (util::Clipboard::HasText()) {
    auto text = util::Clipboard::GetText();
    InsertText(text, text.size());
    ScrollIfNeeded(true, true);
    listener->OnChange();
  }
}

void TextView::Delete() {
  if (selection.IsSelected()) {
    selection.RemoveContent();
    listener->OnChange();
  }
}

void TextView::Undo() {
  if (CanUndo()) {
    undo_stack.Undo(this);
    listener->OnChange();
  }
}

void TextView::Redo() {
  if (CanRedo()) {
    undo_stack.Redo(this);
    listener->OnChange();
  }
}

bool TextView::MouseDown(const Point& point, int button, int clicks,
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

      if (caret.pos.block) {
        mousedown_fragment = caret.pos.block->FindFragment(
            mousedown_point.x, mousedown_point.y - caret.pos.block->ypos);
      }
    }
    caret.ResetBlink();
  }
  return true;
}

bool TextView::MouseUp(const Point& point, int button,
                       ModifierKeys modifierkeys, bool touch) {
  if (button != 1) return false;

  if (touch && !Element::cancel_click) {
    selection.SelectNothing();
    caret.Place(mousedown_point);
    caret.UpdateWantedX();
    caret.ResetBlink();
  }

  select_state = 0;
  if (caret.pos.block && !Element::cancel_click) {
    TextFragment* fragment = caret.pos.block->FindFragment(
        point.x + scroll_x, point.y + scroll_y - caret.pos.block->ypos);
    if (fragment && fragment == mousedown_fragment) {
      fragment->Click(button, modifierkeys);
    }
  }
  return true;
}

bool TextView::MouseMove(const Point& point) {
  if (select_state) {
    Point p(point.x + scroll_x, point.y + scroll_y);
    selection.Select(mousedown_point, p);

    if (select_state == 2) {
      bool has_initial_selection = selection.IsSelected();

      if (has_initial_selection) {
        caret.Place(selection.start.block, selection.start.ofs);
      }
      caret.Move(false, true);
      selection.start.Set(caret.pos);

      if (has_initial_selection) {
        caret.Place(selection.stop.block, selection.stop.ofs);
      }
      caret.Move(true, true);
      selection.stop.Set(caret.pos);

      selection.CorrectOrder();
      caret.UpdateWantedX();
    }
    return true;
  }
  return false;
}

void TextView::Focus(bool focus) {
  if (focus) {
    listener->CaretBlinkStart();
  } else {
    listener->CaretBlinkStop();
  }

  caret.on = focus;
  caret.Invalidate();
  selection.Invalidate();
}

void TextView::set_text(const char* text, CaretPosition pos) {
  set_text(text, strlen(text), pos);
}

void TextView::set_text(const char* text, size_t text_len, CaretPosition pos) {
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

bool TextView::Load(const char* filename) {
  auto file = util::File::Open(filename, util::File::Mode::kRead);
  if (!file) {
    return false;
  }
  size_t num_bytes = file->Size();
  std::vector<char> str(num_bytes + 1);
  num_bytes = file->Read(str.data(), 1, num_bytes);
  str[num_bytes] = 0;
  set_text(str.data());
  return true;
}

std::string TextView::text() {
  TextSelection tmp_selection(this);
  tmp_selection.SelectAll();
  return tmp_selection.text();
}

bool TextView::empty() const {
  return blocks.GetFirst() == blocks.GetLast() &&
         blocks.GetFirst()->str.empty();
}

void TextView::set_alignment(TextAlign align) {
  this->align = align;
  // Call SetAlign on all blocks currently selected, or the block of the current
  // caret position.
  TextBlock* start =
      selection.IsSelected() ? selection.start.block : caret.pos.block;
  TextBlock* stop =
      selection.IsSelected() ? selection.stop.block : caret.pos.block;
  while (start && start != stop->GetNext()) {
    start->set_alignment(align);
    start = start->GetNext();
  }
}

void TextView::set_multiline(bool multiline) {
  packed.multiline_on = multiline;
}

void TextView::set_styled(bool styling) { packed.styling_on = styling; }

void TextView::set_read_only(bool readonly) { packed.read_only = readonly; }

void TextView::set_selection(bool selection) {
  packed.selection_on = selection;
}

void TextView::set_password(bool password) {
  if (packed.password_on == password) return;
  packed.password_on = password;
  Reformat(true);
}

void TextView::set_wrapping(bool wrapping) {
  if (packed.wrapping == wrapping) return;
  packed.wrapping = wrapping;
  Reformat(false);
}

}  // namespace text
}  // namespace el
