/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_TEXT_FRAGMENT_H_
#define EL_TEXT_TEXT_FRAGMENT_H_

#include <string>

#include "el/color.h"
#include "el/element.h"
#include "el/font_description.h"
#include "el/util/intrusive_list.h"
#include "el/util/rect_region.h"

namespace el {
namespace text {

class FontFace;
class TextView;
class TextFragment;
class TextFragmentContent;
class TextFragmentContentFactory;

// A stack of properties used during layout & paint of TextView.
class TextProps {
 public:
  class Data : public el::util::IntrusiveListEntry<Data> {
   public:
    FontDescription font_desc;
    Color text_color;
    bool underline;
  };
  TextProps(const FontDescription& font_desc, const Color& text_color);

  Data* Push();
  void Pop();

  // Gets the font face from the current font description.
  FontFace* computed_font();

 public:
  el::util::AutoDeleteIntrusiveList<Data> data_list;
  Data base_data;
  Data* data;
};

// A block of text (a line, that might be wrapped).
class TextBlock : public el::util::IntrusiveListEntry<TextBlock> {
 public:
  TextBlock(TextView* style_edit);
  ~TextBlock();

  void Clear();
  void Set(const char* newstr, size_t len);
  void set_alignment(TextAlign align);

  size_t InsertText(size_t ofs, const char* text, size_t len,
                    bool allow_line_recurse);
  void RemoveContent(size_t ofs, size_t len);

  // Checks if this block contains extra line breaks and split into new blocks
  // if it does.
  void Split();

  // Checks if we've lost the ending break on this block and if so merge it with
  // the next block.
  void Merge();

  // Lays out the block. To be called when the text has changed or the layout
  // width has changed.
  // @param update_fragments Should be true if the text has been changed (will
  // recreate elements).
  // @param propagate_height If true, all following blocks will be moved if the
  // height changed.
  void Layout(bool update_fragments, bool propagate_height);

  // Updates the size of this block. If propagate_height is true, all following
  // blocks will be moved if the height changed.
  void SetSize(int32_t old_w, int32_t new_w, int32_t new_h,
               bool propagate_height);

  TextFragment* FindFragment(size_t ofs, bool prefer_first = false) const;
  TextFragment* FindFragment(int32_t x, int32_t y) const;

  int32_t CalculateStringWidth(el::text::FontFace* font, const char* str,
                               size_t len = std::string::npos) const;
  int32_t CalculateTabWidth(el::text::FontFace* font, int32_t xpos) const;
  int32_t CalculateLineHeight(text::FontFace* font) const;
  int32_t CalculateBaseline(el::text::FontFace* font) const;

  void Invalidate();
  void BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                            TextProps* props, el::util::RectRegion* bg_region,
                            el::util::RectRegion* fg_region);
  void Paint(int32_t translate_x, int32_t translate_y, TextProps* props);

 public:
  TextView* style_edit;
  el::util::AutoDeleteIntrusiveList<TextFragment> fragments;

  int32_t ypos = 0;
  int16_t height = 0;
  int8_t align = 0;
  int line_width_max = 0;

  std::string str;
  size_t str_len = 0;

 private:
  int GetStartIndentation(text::FontFace* font, size_t first_line_len) const;
};

// The text fragment base class for TextView.
class TextFragment : public el::util::IntrusiveListEntry<TextFragment> {
  // TODO: This object is allocated on vast amounts and need
  // to shrink in size.Remove all cached positioning
  // and implement a fragment traverser(for TextBlock).
  // Also allocate fragments in chunks.
 public:
  TextFragment(TextFragmentContent* content = nullptr) : content(content) {}
  ~TextFragment();

  void Init(TextBlock* block, uint16_t ofs, uint16_t len);

  void UpdateContentPos();

  void BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                            TextProps* props, el::util::RectRegion* bg_region,
                            el::util::RectRegion* fg_region);
  void Paint(int32_t translate_x, int32_t translate_y, TextProps* props);
  void Click(int button, ModifierKeys modifierkeys);

  bool IsText() const { return !IsEmbedded(); }
  bool IsEmbedded() const { return content ? true : false; }
  bool IsBreak() const;
  bool IsSpace() const;
  bool IsTab() const;

  int32_t GetCharX(el::text::FontFace* font, size_t ofs);
  size_t GetCharOfs(el::text::FontFace* font, int32_t x);

  // Gets the string width. Handles password mode, tab, linebreaks etc
  // automatically.
  int32_t GetStringWidth(el::text::FontFace* font, const char* str, size_t len);

  bool GetAllowBreakBefore() const;
  bool GetAllowBreakAfter() const;

  const char* Str() const { return block->str.c_str() + ofs; }

  int32_t GetWidth(el::text::FontFace* font);
  int32_t GetHeight(el::text::FontFace* font);
  int32_t GetBaseline(el::text::FontFace* font);

 public:
  int16_t xpos = 0, ypos = 0;
  uint16_t ofs = 0, len = 0;
  uint16_t line_ypos = 0;
  uint16_t line_height = 0;
  TextBlock* block = nullptr;
  TextFragmentContent* content = nullptr;
};

// Creates TextFragmentContent if the sequence of text matches known content.
class TextFragmentContentFactory {
 public:
  virtual ~TextFragmentContentFactory() = default;

  // Returns then length of the text that represents content that can be
  // created by this factory, or 0 there's no match with any content.
  // F.ex if we can create contet for "<u>" it should return 3 if that is the
  // beginning of text. That length will be consumed from the text output for
  // the created content.
  virtual int GetContent(const char* text);

  // Creates content for a string previosly consumed by calling GetContent.
  virtual TextFragmentContent* CreateFragmentContent(const char* text,
                                                     size_t text_len);
};

}  // namespace text
}  // namespace el

#endif  // EL_TEXT_TEXT_FRAGMENT_H_
