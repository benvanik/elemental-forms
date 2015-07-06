/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/text/font_manager.h"
#include "el/text/text_fragment.h"
#include "el/text/text_fragment_content.h"
#include "el/text/utf8.h"
#include "el/util/string.h"
#include "el/util/string_builder.h"

namespace el {
namespace text {

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

// Checks if no line wrapping is allowed before the character at the given
// offset. The string must be null terminated.
bool is_never_break_before(const char* str, int ofs) {
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
      return ofs > 0 && !util::is_space(str[ofs - 1]);
    default:
      return false;
  }
}

// Checks if no line wrapping is allowed after the character at the given
// offset. The string must be null terminated.
bool is_never_break_after(const char* str, int ofs) {
  switch (str[ofs]) {
    case '(':
    case '[':
    case '{':
    case '<':
      return true;
    case '\'':
    case '"':
      // Simple test if it's the last quote in a word surrounded by space.
      return !util::is_space(str[ofs + 1]);
    default:
      return false;
  }
}

bool GetNextFragment(const char* text,
                     TextFragmentContentFactory* content_factory,
                     size_t* frag_len, bool* is_embed) {
  if (text[0] == '\t') {
    *frag_len = 1;
    return text[1] != 0;
  } else if (text[0] == 0) {
    // Happens when not setting text and maybe when setting "".
    *frag_len = 0;
    return false;
  } else if (text[0] == '\r' || text[0] == '\n') {
    size_t len = (text[0] == '\r' && text[1] == '\n') ? 2 : 1;
    *frag_len = len;
    return false;
  } else if (content_factory) {
    if (size_t content_len = content_factory->GetContent(text)) {
      *frag_len = content_len;
      *is_embed = true;
      return text[content_len] != 0;
    }
  }
  size_t i = 0;
  while (!util::is_wordbreak(text[i])) {
    ++i;
  }
  if (i == 0) {
    if (util::is_wordbreak(text[i])) {
      ++i;
    }
  }
  *frag_len = i;
  if (text[i] == 0) {
    return false;
  }
  return true;
}

TextProps::TextProps(const FontDescription& font_desc,
                     const Color& text_color) {
  base_data.font_desc = font_desc;
  base_data.text_color = text_color;
  base_data.underline = false;
  data = &base_data;
}

TextProps::Data* TextProps::Push() {
  Data* new_data = new Data();
  data_list.AddLast(new_data);
  new_data->font_desc = data->font_desc;
  new_data->text_color = data->text_color;
  new_data->underline = data->underline;
  data = new_data;
  return data;
}

void TextProps::Pop() {
  if (!data_list.GetLast()) {
    return;  // Unbalanced.
  }
  data_list.Delete(data_list.GetLast());
  data = data_list.GetLast() ? data_list.GetLast() : &base_data;
}

el::text::FontFace* TextProps::computed_font() {
  return el::text::FontManager::get()->GetFontFace(data->font_desc);
}

TextBlock::TextBlock(TextView* style_edit)
    : style_edit(style_edit), align(int8_t(style_edit->align)) {}

TextBlock::~TextBlock() { Clear(); }

void TextBlock::Clear() { fragments.DeleteAll(); }

void TextBlock::Set(const char* newstr, size_t len) {
  str.assign(newstr, len);
  str_len = len;
  Split();
  Layout(true, true);
}

void TextBlock::set_alignment(TextAlign align) {
  if (TextAlign(this->align) == align) return;
  this->align = int8_t(align);
  Layout(false, false);
}

size_t TextBlock::InsertText(size_t ofs, const char* text, size_t len,
                             bool allow_line_recurse) {
  style_edit->BeginLockScrollbars();
  size_t first_line_len = len;
  for (size_t i = 0; i < len; i++)
    if (text[i] == '\r' || text[i] == '\n') {
      first_line_len = i;
      // Include the line break too but not for single lines
      if (!style_edit->packed.multiline_on) break;
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
  if (allow_line_recurse && style_edit->packed.multiline_on) {
    // Instead of recursively calling InsertText, we will loop through them all
    // here.
    TextBlock* next_block = GetNext();
    const char* next_line_ptr = &text[first_line_len];
    size_t remaining = len - first_line_len;
    while (remaining > 0) {
      if (!next_block) {
        next_block = new TextBlock(style_edit);
        style_edit->blocks.AddLast(next_block);
      }
      size_t consumed =
          next_block->InsertText(0, next_line_ptr, remaining, false);
      next_line_ptr += consumed;
      inserted_len += consumed;
      remaining -= consumed;
      next_block = next_block->GetNext();
    }
  }
  style_edit->EndLockScrollbars();
  return inserted_len;
}

void TextBlock::RemoveContent(size_t ofs, size_t len) {
  if (!len) return;
  str.erase(ofs, len);
  str_len -= len;
  Layout(true, true);
}

void TextBlock::Split() {
  if (!str_len) {
    return;
  }
  size_t len = str_len;
  size_t brlen = 1;  // FIX: skip ending newline fragment but not if there is
                     // several newlines and check for singleline newline.
  if (len > 1 && str[len - 2] == '\r' && str[len - 1] == '\n') {
    ++brlen;
  }
  len -= brlen;
  for (size_t i = 0; i < len; ++i) {
    if (util::is_linebreak(str[i])) {
      TextBlock* block = new TextBlock(style_edit);
      style_edit->blocks.AddAfter(block, this);

      if (i < len - 1 && str[i] == '\r' && str[i + 1] == '\n') {
        ++i;
      }
      i++;

      len = len + brlen - i;
      block->Set(str.c_str() + i, len);
      str.erase(i, len);
      str_len -= len;
      break;
    }
  }
}

void TextBlock::Merge() {
  TextBlock* next_block = GetNext();
  if (next_block && !fragments.GetLast()->IsBreak()) {
    str.append(GetNext()->str);
    str_len = str.size();

    style_edit->blocks.Delete(next_block);

    height = 0;  // Ensure that Layout propagate height to remaining blocks.
    Layout(true, true);
  }
}

int32_t TextBlock::CalculateTabWidth(el::text::FontFace* font,
                                     int32_t xpos) const {
  int tabsize = font->GetStringWidth("x", 1) * TAB_SPACE;
  int p2 = int(xpos / tabsize) * tabsize + tabsize;
  return p2 - xpos;
}

int32_t TextBlock::CalculateStringWidth(el::text::FontFace* font,
                                        const char* str, size_t len) const {
  if (style_edit->packed.password_on) {
    // Convert the length in number or characters, since that's what matters for
    // password width.
    len = utf8::count_characters(str, len);
    return font->GetStringWidth(special_char_password) * int(len);
  }
  return font->GetStringWidth(str, len);
}

int32_t TextBlock::CalculateLineHeight(el::text::FontFace* font) const {
  return font->height();
}

int32_t TextBlock::CalculateBaseline(el::text::FontFace* font) const {
  return font->ascent();
}

int TextBlock::GetStartIndentation(el::text::FontFace* font,
                                   size_t first_line_len) const {
  // Lines beginning with whitespace or list points, should
  // indent to the same as the beginning when wrapped.
  int32_t indentation = 0;
  size_t i = 0;
  while (i < first_line_len) {
    const char* current_str = str.c_str() + i;
    auto uc = utf8::decode_next(str.c_str(), &i, first_line_len);
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

void TextBlock::Layout(bool update_fragments, bool propagate_height) {
  // Create fragments from the word fragments.
  if (update_fragments || !fragments.GetFirst()) {
    Clear();

    size_t ofs = 0;
    const char* text = str.c_str();
    while (true) {
      size_t frag_len;
      bool is_embed = false;
      bool more = GetNextFragment(
          &text[ofs],
          style_edit->packed.styling_on ? style_edit->content_factory : nullptr,
          &frag_len, &is_embed);

      TextFragment* fragment = new TextFragment();
      fragment->Init(this, uint16_t(ofs), uint16_t(frag_len));

      if (is_embed) {
        fragment->content = style_edit->content_factory->CreateFragmentContent(
            &text[ofs], frag_len);
      }

      fragments.AddLast(fragment);
      ofs += frag_len;

      if (!more) break;
    }
  }

  // Layout.
  if (style_edit->layout_width <= 0 && style_edit->GetSizeAffectsLayout()) {
    // Don't layout if we have no space. This will happen when setting text
    // before the element has been layouted. We will relayout when we are
    // resized.
    return;
  }

  int old_line_width_max = line_width_max;
  line_width_max = 0;
  int line_ypos = 0;
  int first_line_indentation = 0;
  TextFragment* first_fragment_on_line = fragments.GetFirst();

  while (first_fragment_on_line) {
    int line_width = 0;

    // Get the last fragment that should be laid out on the line while
    // calculating line width and preliminary x positions for the fragments.
    TextFragment* last_fragment_on_line = fragments.GetLast();
    if (style_edit->packed.wrapping) {
      // If we should wrap, search for the last allowed break point before the
      // overflow.
      TextFragment* allowed_last_fragment = nullptr;

      int line_xpos = first_line_indentation;
      for (TextFragment* fragment = first_fragment_on_line; fragment;
           fragment = fragment->GetNext()) {
        // Give the fragment the current x. Then tab widths are calculated
        // properly in GetWidth.
        fragment->xpos = line_xpos;
        int fragment_w = fragment->GetWidth(style_edit->font);

        // Check if we overflow.
        bool overflow = line_xpos + fragment_w > style_edit->layout_width;

        if (overflow && allowed_last_fragment) {
          last_fragment_on_line = allowed_last_fragment;
          break;
        }

        // Check if this is a allowed break position.
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
      for (TextFragment* fragment = first_fragment_on_line; fragment;
           fragment = fragment->GetNext()) {
        fragment->xpos = line_width;
        line_width += fragment->GetWidth(style_edit->font);
      }
    }

    // Commit line - Layout each fragment on the line.
    int line_height = 0;
    int line_baseline = 0;
    TextFragment* fragment = first_fragment_on_line;
    while (fragment) {
      line_height =
          std::max(fragment->GetHeight(style_edit->font), line_height);
      line_baseline =
          std::max(fragment->GetBaseline(style_edit->font), line_baseline);

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
      xofs = style_edit->layout_width - line_width;
    } else if (TextAlign(align) == TextAlign::kCenter) {
      xofs = (style_edit->layout_width - line_width) / 2;
    }

    int adjusted_line_height = line_height;
    fragment = first_fragment_on_line;
    while (fragment) {
      // The fragment need to know these later.
      fragment->line_ypos = line_ypos;
      fragment->line_height = line_height;

      // Adjust the position.
      fragment->ypos += line_baseline - fragment->GetBaseline(style_edit->font);
      fragment->xpos += xofs;

      // We now know the final position so update content.
      fragment->UpdateContentPos();

      // Total line height may now have changed a bit.
      adjusted_line_height =
          std::max(line_baseline - fragment->GetBaseline(style_edit->font) +
                       fragment->GetHeight(style_edit->font),
                   adjusted_line_height);

      if (fragment == last_fragment_on_line) {
        break;
      }
      fragment = fragment->GetNext();
    }

    // Update line_height set on fragments if needed.
    if (line_height != adjusted_line_height) {
      for (fragment = first_fragment_on_line;
           fragment != last_fragment_on_line->GetNext();
           fragment = fragment->GetNext()) {
        fragment->line_height = adjusted_line_height;
      }
    }

    line_width_max = std::max(line_width_max, line_width);

    // This was the first line so calculate the indentation to use for the other
    // lines.
    if (style_edit->packed.wrapping &&
        first_fragment_on_line == fragments.GetFirst()) {
      first_line_indentation =
          GetStartIndentation(style_edit->font, last_fragment_on_line->ofs +
                                                    last_fragment_on_line->len);
    }

    // Consume line.
    line_ypos += adjusted_line_height;

    first_fragment_on_line = last_fragment_on_line->GetNext();
  }

  ypos = GetPrev() ? GetPrev()->ypos + GetPrev()->height : 0;
  SetSize(old_line_width_max, line_width_max, line_ypos, propagate_height);

  Invalidate();
}

void TextBlock::SetSize(int32_t old_w, int32_t new_w, int32_t new_h,
                        bool propagate_height) {
  // Later: could optimize with Scroll here.
  int32_t dh = new_h - height;
  height = new_h;
  if (dh != 0 && propagate_height) {
    TextBlock* block = GetNext();
    while (block) {
      block->ypos = block->GetPrev()->ypos + block->GetPrev()->height;
      block->Invalidate();
      block = block->GetNext();
    }
  }

  // Update content_width and content_height.
  // content_width can only be calculated in constant time if we grow larger.
  // If we shrink our width and where equal to content_width, we don't know
  // how wide the widest block is and we set a flag to update it when needed.

  if (!style_edit->packed.wrapping && !style_edit->packed.multiline_on) {
    style_edit->content_width = new_w;
  } else if (new_w > style_edit->content_width) {
    style_edit->content_width = new_w;
  } else if (new_w < old_w && old_w == style_edit->content_width) {
    style_edit->packed.calculate_content_width_needed = 1;
  }

  style_edit->content_height =
      style_edit->blocks.GetLast()->ypos + style_edit->blocks.GetLast()->height;

  if (style_edit->listener && style_edit->packed.lock_scrollbars_counter == 0 &&
      propagate_height) {
    style_edit->listener->UpdateScrollbars();
  }
}

TextFragment* TextBlock::FindFragment(size_t ofs, bool prefer_first) const {
  TextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    if (prefer_first && ofs <= fragment->ofs + fragment->len) return fragment;
    if (!prefer_first && ofs < fragment->ofs + fragment->len) return fragment;
    fragment = fragment->GetNext();
  }
  return fragments.GetLast();
}

TextFragment* TextBlock::FindFragment(int32_t x, int32_t y) const {
  TextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    if (y < fragment->line_ypos + fragment->line_height) {
      if (x < fragment->xpos + fragment->GetWidth(style_edit->font)) {
        return fragment;
      }
      if (fragment->GetNext() &&
          fragment->GetNext()->line_ypos > fragment->line_ypos) {
        return fragment;
      }
    }
    fragment = fragment->GetNext();
  }
  return fragments.GetLast();
}

void TextBlock::Invalidate() {
  if (style_edit->listener) {
    style_edit->listener->Invalidate(Rect(0, -style_edit->scroll_y + ypos,
                                          style_edit->layout_width, height));
  }
}

void TextBlock::BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                                     TextProps* props,
                                     util::RectRegion* bg_region,
                                     util::RectRegion* fg_region) {
  if (!style_edit->selection.IsBlockSelected(this)) return;

  TextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    fragment->BuildSelectionRegion(translate_x, translate_y + ypos, props,
                                   bg_region, fg_region);
    fragment = fragment->GetNext();
  }
}

void TextBlock::Paint(int32_t translate_x, int32_t translate_y,
                      TextProps* props) {
  TMPDEBUG(style_edit->listener->DrawRect(
      Rect(translate_x, translate_y + ypos, style_edit->layout_width, height),
      Color(255, 200, 0, 128)));

  TextFragment* fragment = fragments.GetFirst();
  while (fragment) {
    fragment->Paint(translate_x, translate_y + ypos, props);
    fragment = fragment->GetNext();
  }
}

TextFragment::~TextFragment() { delete content; }

void TextFragment::Init(TextBlock* block, uint16_t ofs, uint16_t len) {
  this->block = block;
  this->ofs = ofs;
  this->len = len;
}

void TextFragment::UpdateContentPos() {
  if (content) {
    content->UpdatePos(xpos, ypos + block->ypos);
  }
}

void TextFragment::BuildSelectionRegion(int32_t translate_x,
                                        int32_t translate_y, TextProps* props,
                                        util::RectRegion* bg_region,
                                        util::RectRegion* fg_region) {
  if (!block->style_edit->selection.IsFragmentSelected(this)) return;

  int x = translate_x + xpos;
  int y = translate_y + ypos;
  auto font = props->computed_font();

  if (content) {
    // Selected embedded content should add to the foreground region.
    fg_region->IncludeRect(Rect(x, y, GetWidth(font), GetHeight(font)));
    return;
  }

  // Selected text should add to the backgroud region.
  TextSelection* sel = &block->style_edit->selection;

  size_t sofs1 = sel->start.block == block ? sel->start.ofs : 0;
  size_t sofs2 = sel->stop.block == block ? sel->stop.ofs : block->str_len;
  sofs1 = std::max(sofs1, size_t(ofs));
  sofs2 = std::min(sofs2, size_t(ofs + len));

  int s1x = GetStringWidth(font, block->str.c_str() + ofs, sofs1 - ofs);
  int s2x = GetStringWidth(font, block->str.c_str() + sofs1, sofs2 - sofs1);

  bg_region->IncludeRect(Rect(x + s1x, y, s2x, GetHeight(font)));
}

void TextFragment::Paint(int32_t translate_x, int32_t translate_y,
                         TextProps* props) {
  TextViewListener* listener = block->style_edit->listener;

  int x = translate_x + xpos;
  int y = translate_y + ypos;
  Color color = props->data->text_color;
  auto font = props->computed_font();

  if (content) {
    content->Paint(this, translate_x, translate_y, props);
    return;
  }
  TMPDEBUG(listener->DrawRect(Rect(x, y, GetWidth(font), GetHeight(font)),
                              Color(255, 255, 255, 128)));

  if (IsBreak()) {
    listener->OnBreak();
  }
  if (block->style_edit->packed.password_on) {
    int cw = block->CalculateStringWidth(font, special_char_password);
    size_t num_char = utf8::count_characters(Str(), len);
    for (size_t i = 0; i < num_char; ++i) {
      listener->DrawString(int(x + i * cw), y, font, color,
                           special_char_password);
    }
  } else if (block->style_edit->packed.show_whitespace) {
    if (IsTab()) {
      listener->DrawString(x, y, font, color, special_char_tab);
    } else if (IsBreak()) {
      listener->DrawString(x, y, font, color, special_char_newln);
    } else if (IsSpace()) {
      listener->DrawString(x, y, font, color, special_char_space);
    } else {
      listener->DrawString(x, y, font, color, Str(), len);
    }
  } else if (!IsTab() && !IsBreak() && !IsSpace()) {
    listener->DrawString(x, y, font, color, Str(), len);
  }

  if (props->data->underline) {
    int line_h = font->height() / 16;
    line_h = std::max(line_h, 1);
    listener->DrawRectFill(
        Rect(x, y + GetBaseline(font) + 1, GetWidth(font), line_h), color);
  }
}

void TextFragment::Click(int button, ModifierKeys modifierkeys) {
  if (content) {
    content->Click(this, button, modifierkeys);
  }
}

int32_t TextFragment::GetWidth(el::text::FontFace* font) {
  if (content) return content->GetWidth(font, this);
  if (IsBreak()) return 0;
  if (IsTab()) return block->CalculateTabWidth(font, xpos);
  return block->CalculateStringWidth(font, block->str.c_str() + ofs, len);
}

int32_t TextFragment::GetHeight(el::text::FontFace* font) {
  if (content) return content->GetHeight(font, this);
  return block->CalculateLineHeight(font);
}

int32_t TextFragment::GetBaseline(el::text::FontFace* font) {
  if (content) return content->GetBaseline(font, this);
  return block->CalculateBaseline(font);
}

int32_t TextFragment::GetCharX(el::text::FontFace* font, size_t ofs) {
  assert(ofs >= 0 && ofs <= len);

  if (IsEmbedded() || IsTab()) {
    return ofs == 0 ? 0 : GetWidth(font);
  }
  if (IsBreak()) {
    return 0;
  }

  return block->CalculateStringWidth(font, block->str.c_str() + this->ofs, ofs);
}

size_t TextFragment::GetCharOfs(el::text::FontFace* font, int32_t x) {
  if (IsEmbedded() || IsTab()) {
    return x > GetWidth(font) / 2 ? 1 : 0;
  }
  if (IsBreak()) {
    return 0;
  }

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
    if (x < width - (width - width_except_last_char) / 2) {
      return pos;
    }
  }
  return len;
}

int32_t TextFragment::GetStringWidth(el::text::FontFace* font, const char* str,
                                     size_t len) {
  if (IsTab()) {
    return len ? block->CalculateTabWidth(font, xpos) : 0;
  }
  if (IsBreak()) {
    return len ? 8 : 0;
  }
  return block->CalculateStringWidth(font, str, len);
}

bool TextFragment::IsBreak() const {
  return Str()[0] == '\r' || Str()[0] == '\n';
}

bool TextFragment::IsSpace() const { return util::is_space(Str()[0]); }

bool TextFragment::IsTab() const { return Str()[0] == '\t'; }

bool TextFragment::GetAllowBreakBefore() const {
  if (content) {
    return content->GetAllowBreakBefore();
  }
  if (len && !is_never_break_before(block->str.c_str(), ofs)) {
    return true;
  }
  return false;
}

bool TextFragment::GetAllowBreakAfter() const {
  if (content) {
    return content->GetAllowBreakAfter();
  }
  if (len && !is_never_break_after(block->str.c_str(), ofs + len - 1)) {
    return true;
  }
  return false;
}

}  // namespace text
}  // namespace el
