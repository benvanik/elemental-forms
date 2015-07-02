/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_style_edit_content.h"

#include <algorithm>
#include <cassert>

#include "tb_style_edit.h"

namespace tb {

int TBTextFragmentContentFactory::GetContent(const char* text) {
  if (text[0] == '<') {
    int i = 0;
    while (text[i] != '>' && text[i] > 31) i++;
    if (text[i] == '>') {
      i++;
      return i;
    }
  }
  return 0;
}

TBTextFragmentContent* TBTextFragmentContentFactory::CreateFragmentContent(
    const char* text, int text_len) {
  if (strncmp(text, "<hr>", text_len) == 0)
    return new TBTextFragmentContentHR(100, 2);
  else if (strncmp(text, "<u>", text_len) == 0)
    return new TBTextFragmentContentUnderline();
  else if (strncmp(text, "<color ", std::min(text_len, 7)) == 0) {
    TBColor color;
    color.SetFromString(text + 7, text_len - 8);
    return new TBTextFragmentContentTextColor(color);
  } else if (strncmp(text, "</", std::min(text_len, 2)) == 0)
    return new TBTextFragmentContentStylePop();
  return nullptr;
}

TBTextFragmentContentHR::TBTextFragmentContentHR(int32_t width_in_percent,
                                                 int32_t height)
    : width_in_percent(width_in_percent), height(height) {}

void TBTextFragmentContentHR::Paint(TBTextFragment* fragment,
                                    int32_t translate_x, int32_t translate_y,
                                    TBTextProps* props) {
  int x = translate_x + fragment->xpos;
  int y = translate_y + fragment->ypos;

  int w = fragment->block->styledit->layout_width * width_in_percent / 100;
  x += (fragment->block->styledit->layout_width - w) / 2;

  TBStyleEditListener* listener = fragment->block->styledit->listener;
  listener->DrawRectFill(Rect(x, y, w, height), props->data->text_color);
}

int32_t TBTextFragmentContentHR::GetWidth(TBFontFace* font,
                                          TBTextFragment* fragment) {
  return std::max(fragment->block->styledit->layout_width, 0);
}

int32_t TBTextFragmentContentHR::GetHeight(TBFontFace* font,
                                           TBTextFragment* fragment) {
  return height;
}

void TBTextFragmentContentUnderline::Paint(TBTextFragment* fragment,
                                           int32_t translate_x,
                                           int32_t translate_y,
                                           TBTextProps* props) {
  if (TBTextProps::Data* data = props->Push()) data->underline = true;
}

void TBTextFragmentContentTextColor::Paint(TBTextFragment* fragment,
                                           int32_t translate_x,
                                           int32_t translate_y,
                                           TBTextProps* props) {
  if (TBTextProps::Data* data = props->Push()) data->text_color = color;
}

void TBTextFragmentContentStylePop::Paint(TBTextFragment* fragment,
                                          int32_t translate_x,
                                          int32_t translate_y,
                                          TBTextProps* props) {
  props->Pop();
}

}  // namespace tb
