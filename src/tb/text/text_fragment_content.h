/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TEXT_TEXT_FRAGMENT_CONTENT_H_
#define TB_TEXT_TEXT_FRAGMENT_CONTENT_H_

#include "tb/text/text_view.h"

namespace tb {
namespace text {

class FontFace;
class TextFragment;

// Content for a non-text TextFragment.
class TextFragmentContent {
 public:
  virtual ~TextFragmentContent() = default;

  // Updates the position of the content, relative to the first line of text (no
  // scrolling applied).
  virtual void UpdatePos(int x, int y) {}

  virtual void Paint(TextFragment* fragment, int32_t translate_x,
                     int32_t translate_y, TextProps* props) {}
  virtual void Click(TextFragment* fragment, int button,
                     ModifierKeys modifierkeys) {}
  virtual int32_t GetWidth(tb::text::FontFace* font, TextFragment* fragment) {
    return 0;
  }
  virtual int32_t GetHeight(tb::text::FontFace* font, TextFragment* fragment) {
    return 0;
  }
  virtual int32_t GetBaseline(tb::text::FontFace* font,
                              TextFragment* fragment) {
    return GetHeight(font, fragment);
  }
  virtual bool GetAllowBreakBefore() { return true; }
  virtual bool GetAllowBreakAfter() { return true; }

  // Gets type of fragment content. All standard fragments return 0.
  virtual uint32_t type() { return 0; }
};

// A horizontal line for TextView.
class TextFragmentContentHR : public TextFragmentContent {
 public:
  TextFragmentContentHR(int32_t width_in_percent, int32_t height);

  void Paint(TextFragment* fragment, int32_t translate_x, int32_t translate_y,
             TextProps* props) override;
  int32_t GetWidth(tb::text::FontFace* font, TextFragment* fragment) override;
  int32_t GetHeight(tb::text::FontFace* font, TextFragment* fragment) override;

 private:
  int32_t width_in_percent_;
  int32_t height_;
};

// Fragment content that enables underline in a TextView.
class TextFragmentContentUnderline : public TextFragmentContent {
 public:
  TextFragmentContentUnderline() = default;
  void Paint(TextFragment* fragment, int32_t translate_x, int32_t translate_y,
             TextProps* props) override;
  bool GetAllowBreakBefore() override { return true; }
  bool GetAllowBreakAfter() override { return false; }
};

// Fragment content that changes color in a TextView.
class TextFragmentContentTextColor : public TextFragmentContent {
 public:
  TextFragmentContentTextColor(const Color& color) : color(color) {}
  void Paint(TextFragment* fragment, int32_t translate_x, int32_t translate_y,
             TextProps* props) override;
  bool GetAllowBreakBefore() override { return true; }
  bool GetAllowBreakAfter() override { return false; }

  Color color;
};

// Fragment content that ends a change of style in a TextView.
class TextFragmentContentStylePop : public TextFragmentContent {
 public:
  void Paint(TextFragment* fragment, int32_t translate_x, int32_t translate_y,
             TextProps* props) override;
  bool GetAllowBreakBefore() override { return false; }
  bool GetAllowBreakAfter() override { return true; }
};

}  // namespace text
}  // namespace tb

#endif  // TB_TEXT_TEXT_FRAGMENT_CONTENT_H_
