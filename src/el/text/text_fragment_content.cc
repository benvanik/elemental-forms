/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>

#include "el/text/text_fragment_content.h"

namespace el {
namespace text {

int TextFragmentContentFactory::GetContent(const char* text) {
  if (text[0] == '<') {
    int i = 0;
    while (text[i] != '>' && text[i] > 31) {
      ++i;
    }
    if (text[i] == '>') {
      ++i;
      return i;
    }
  }
  return 0;
}

TextFragmentContent* TextFragmentContentFactory::CreateFragmentContent(
    const char* text, size_t text_len) {
  if (strncmp(text, "<hr>", text_len) == 0) {
    return new TextFragmentContentHR(100, 2);
  } else if (strncmp(text, "<u>", text_len) == 0) {
    return new TextFragmentContentUnderline();
  } else if (strncmp(text, "<color ", std::min(text_len, size_t(7))) == 0) {
    Color color;
    color.reset(text + 7, text_len - 8);
    return new TextFragmentContentTextColor(color);
  } else if (strncmp(text, "</", std::min(text_len, size_t(2))) == 0) {
    return new TextFragmentContentStylePop();
  }
  return nullptr;
}

TextFragmentContentHR::TextFragmentContentHR(int32_t width_in_percent,
                                             int32_t height)
    : width_in_percent_(width_in_percent), height_(height) {}

void TextFragmentContentHR::Paint(TextFragment* fragment, int32_t translate_x,
                                  int32_t translate_y, TextProps* props) {
  int x = translate_x + fragment->xpos;
  int y = translate_y + fragment->ypos;

  int w = fragment->block->style_edit->layout_width * width_in_percent_ / 100;
  x += (fragment->block->style_edit->layout_width - w) / 2;

  TextViewListener* listener = fragment->block->style_edit->listener;
  listener->DrawRectFill(Rect(x, y, w, height_), props->data->text_color);
}

int32_t TextFragmentContentHR::GetWidth(text::FontFace* font,
                                        TextFragment* fragment) {
  return std::max(fragment->block->style_edit->layout_width, 0);
}

int32_t TextFragmentContentHR::GetHeight(text::FontFace* font,
                                         TextFragment* fragment) {
  return height_;
}

void TextFragmentContentUnderline::Paint(TextFragment* fragment,
                                         int32_t translate_x,
                                         int32_t translate_y,
                                         TextProps* props) {
  if (TextProps::Data* data = props->Push()) {
    data->underline = true;
  }
}

void TextFragmentContentTextColor::Paint(TextFragment* fragment,
                                         int32_t translate_x,
                                         int32_t translate_y,
                                         TextProps* props) {
  if (TextProps::Data* data = props->Push()) {
    data->text_color = color;
  }
}

void TextFragmentContentStylePop::Paint(TextFragment* fragment,
                                        int32_t translate_x,
                                        int32_t translate_y, TextProps* props) {
  props->Pop();
}

}  // namespace text
}  // namespace el
