/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_STYLE_EDIT_H
#define TB_STYLE_EDIT_H

#include <algorithm>

#include "tb_core.h"
#include "tb_linklist.h"
#include "tb_list.h"
#include "tb_widgets_common.h"

namespace tb {

class TextBlock;
class StyleEdit;
class TextFragment;
class TextFragmentContent;
class TextFragmentContentFactory;

namespace util {
class RectRegion;
}  // namespace util

// Listener for StyleEdit. Implement in the environment the StyleEdit should
// render its content.
class StyleEditListener {
 public:
  virtual ~StyleEditListener() = default;

  virtual void OnChange() {}
  virtual bool OnEnter() { return false; }
  virtual void Invalidate(const Rect& rect) = 0;
  virtual void DrawString(int32_t x, int32_t y, FontFace* font,
                          const Color& color, const char* str,
                          size_t len = std::string::npos) = 0;
  virtual void DrawRect(const Rect& rect, const Color& color) = 0;
  virtual void DrawRectFill(const Rect& rect, const Color& color) = 0;
  virtual void DrawTextSelectionBg(const Rect& rect) = 0;
  virtual void DrawContentSelectionFg(const Rect& rect) = 0;
  virtual void DrawCaret(const Rect& rect) = 0;
  virtual void Scroll(int32_t dx, int32_t dy) = 0;
  virtual void UpdateScrollbars() = 0;
  virtual void CaretBlinkStart() = 0;
  virtual void CaretBlinkStop() = 0;
  virtual void OnBreak() {}
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

class TextOffset {
 public:
  TextOffset() = default;
  TextOffset(TextBlock* block, size_t ofs) : block(block), ofs(ofs) {}

  void Set(TextBlock* new_block, size_t new_ofs) {
    block = new_block;
    ofs = new_ofs;
  }
  void Set(const TextOffset& pos) {
    block = pos.block;
    ofs = pos.ofs;
  }

  size_t GetGlobalOffset(StyleEdit* se) const;
  bool SetGlobalOffset(StyleEdit* se, size_t gofs);

 public:
  TextBlock* block = nullptr;
  size_t ofs = 0;
};

// Handles the selected text in a StyleEdit.
class TextSelection {
 public:
  TextSelection(StyleEdit* style_edit);
  void Invalidate() const;
  void Select(const TextOffset& new_start, const TextOffset& new_stop);
  void Select(const Point& from, const Point& to);
  void Select(size_t glob_ofs_from, size_t glob_ofs_to);
  void SelectToCaret(TextBlock* old_caret_block, size_t old_caret_ofs);
  void SelectAll();
  void SelectNothing();
  void CorrectOrder();
  void CopyToClipboard();
  bool IsBlockSelected(TextBlock* block) const;
  bool IsFragmentSelected(TextFragment* elm) const;
  bool IsSelected() const;
  void RemoveContent();
  std::string GetText() const;

 public:
  StyleEdit* style_edit;
  TextOffset start;
  TextOffset stop;
};

enum class CaretPosition {
  kBeginning,
  kEnd,
};

// The caret in a StyleEdit.
class Caret {
 public:
  Caret(StyleEdit* style_edit);
  void Invalidate();
  void UpdatePos();
  bool Move(bool forward, bool word);
  bool Place(const Point& point);
  bool Place(TextBlock* block, size_t ofs, bool allow_snap = true,
             bool snap_forward = false);
  void Place(CaretPosition place);
  void AvoidLineBreak();
  void Paint(int32_t translate_x, int32_t translate_y);
  void ResetBlink();
  void UpdateWantedX();

  size_t GetGlobalOffset() const { return pos.GetGlobalOffset(style_edit); }
  void SetGlobalOffset(size_t gofs, bool allow_snap = true,
                       bool snap_forward = false);

  TextFragment* GetFragment();

 private:
  void SwitchBlock(bool second);

 public:
  StyleEdit* style_edit;
  int32_t x = 0, y = 0;  // Relative to the style_edit.
  int32_t width = 2;
  int32_t height = 0;
  int32_t wanted_x = 0;
  bool on = false;
  bool prefer_first = true;
  TextOffset pos;
};

// A stack of properties used during layout & paint of StyleEdit.
class TextProps {
 public:
  class Data : public TBLinkOf<Data> {
   public:
    FontDescription font_desc;
    Color text_color;
    bool underline;
  };
  TextProps(const FontDescription& font_desc, const Color& text_color);

  Data* Push();
  void Pop();

  // Gets the font face from the current font description.
  FontFace* GetFont();

 public:
  TBLinkListOf<Data> data_list;
  Data base_data;
  Data* data;
};

// A block of text (a line, that might be wrapped).
class TextBlock : public TBLinkOf<TextBlock> {
 public:
  TextBlock(StyleEdit* style_edit);
  ~TextBlock();

  void Clear();
  void Set(const char* newstr, size_t len);
  void SetAlign(TextAlign align);

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

  int32_t CalculateStringWidth(FontFace* font, const char* str,
                               size_t len = std::string::npos) const;
  int32_t CalculateTabWidth(FontFace* font, int32_t xpos) const;
  int32_t CalculateLineHeight(FontFace* font) const;
  int32_t CalculateBaseline(FontFace* font) const;

  void Invalidate();
  void BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                            TextProps* props, util::RectRegion* bg_region,
                            util::RectRegion* fg_region);
  void Paint(int32_t translate_x, int32_t translate_y, TextProps* props);

 public:
  StyleEdit* style_edit;
  TBLinkListOf<TextFragment> fragments;

  int32_t ypos = 0;
  int16_t height = 0;
  int8_t align = 0;
  int line_width_max = 0;

  std::string str;
  size_t str_len = 0;

 private:
  int GetStartIndentation(FontFace* font, size_t first_line_len) const;
};

// Event in the UndoRedoStack. Each insert or remove change is stored as a
// UndoEvent, but they may also be merged when appropriate.
class UndoEvent {
 public:
  size_t gofs;
  std::string text;
  bool insert;
};

// Keeps track of all UndoEvents used for undo and redo functionality.
class UndoRedoStack {
 public:
  UndoRedoStack() = default;
  ~UndoRedoStack();

  void Undo(StyleEdit* style_edit);
  void Redo(StyleEdit* style_edit);
  void Clear(bool clear_undo, bool clear_redo);

  UndoEvent* Commit(StyleEdit* style_edit, size_t gofs, size_t len,
                    const char* text, bool insert);

 public:
  TBListOf<UndoEvent> undos;
  TBListOf<UndoEvent> redos;
  bool applying = false;

 private:
  void Apply(StyleEdit* style_edit, UndoEvent* e, bool reverse);
};

// The text fragment base class for StyleEdit.
class TextFragment : public TBLinkOf<TextFragment> {
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
                            TextProps* props, util::RectRegion* bg_region,
                            util::RectRegion* fg_region);
  void Paint(int32_t translate_x, int32_t translate_y, TextProps* props);
  void Click(int button, ModifierKeys modifierkeys);

  bool IsText() const { return !IsEmbedded(); }
  bool IsEmbedded() const { return content ? true : false; }
  bool IsBreak() const;
  bool IsSpace() const;
  bool IsTab() const;

  int32_t GetCharX(FontFace* font, size_t ofs);
  size_t GetCharOfs(FontFace* font, int32_t x);

  // Gets the string width. Handles password mode, tab, linebreaks etc
  // automatically.
  int32_t GetStringWidth(FontFace* font, const char* str, size_t len);

  bool GetAllowBreakBefore() const;
  bool GetAllowBreakAfter() const;

  const char* Str() const { return block->str.c_str() + ofs; }

  int32_t GetWidth(FontFace* font);
  int32_t GetHeight(FontFace* font);
  int32_t GetBaseline(FontFace* font);

 public:
  int16_t xpos = 0, ypos = 0;
  uint16_t ofs = 0, len = 0;
  uint16_t line_ypos = 0;
  uint16_t line_height = 0;
  TextBlock* block = nullptr;
  TextFragmentContent* content = nullptr;
};

// Edits and formats TextFragment's. It handles the text in a StyleEditView.
class StyleEdit {
 public:
  StyleEdit();
  virtual ~StyleEdit();

  void SetListener(StyleEditListener* listener);
  void SetContentFactory(TextFragmentContentFactory* content_factory);

  void SetFont(const FontDescription& font_desc);

  void Paint(const Rect& rect, const FontDescription& font_desc,
             const Color& text_color);
  bool KeyDown(int key, SpecialKey special_key, ModifierKeys modifierkeys);
  bool MouseDown(const Point& point, int button, int clicks,
                 ModifierKeys modifierkeys, bool touch);
  bool MouseUp(const Point& point, int button, ModifierKeys modifierkeys,
               bool touch);
  bool MouseMove(const Point& point);
  void Focus(bool focus);

  void Clear(bool init_new = true);
  bool Load(const char* filename);
  void SetText(const char* text, CaretPosition pos = CaretPosition::kBeginning);
  void SetText(const char* text, size_t text_len,
               CaretPosition pos = CaretPosition::kBeginning);
  std::string GetText();
  bool empty() const;

  // Sets the default text alignment and all currently selected blocks, or the
  // block of the current caret position if nothing is selected.
  void SetAlign(TextAlign align);
  void SetMultiline(bool multiline = true);
  void SetStyling(bool styling = true);
  void SetReadOnly(bool readonly = true);
  void SetSelection(bool selection = true);
  void SetPassword(bool password = true);
  void SetWrapping(bool wrapping = true);

  // Sets if line breaks should be inserted in windows style (\r\n) or unix
  // style (\n). The default is windows style on the windows platform and
  // disabled elsewhere.
  // NOTE: This only affects InsertBreak (pressing enter). Content set from
  // SetText (and clipboard etc.) maintains the used line break.
  void SetWindowsStyleBreak(bool win_style_br) {
    packed.win_style_br = win_style_br;
  }

  void Cut();
  void Copy();
  void Paste();
  void Delete();

  void Undo();
  void Redo();
  bool CanUndo() const { return undoredo.undos.GetNumItems() ? true : false; }
  bool CanRedo() const { return undoredo.redos.GetNumItems() ? true : false; }

  void InsertText(const char* text, size_t len = std::string::npos,
                  bool after_last = false, bool clear_undo_redo = false);
  void InsertText(const std::string& text, size_t len = std::string::npos,
                  bool after_last = false, bool clear_undo_redo = false) {
    InsertText(text.c_str(), len, after_last, clear_undo_redo);
  }
  void AppendText(const char* text, size_t len = std::string::npos,
                  bool clear_undo_redo = false) {
    InsertText(text, len, true, clear_undo_redo);
  }
  void InsertBreak();

  TextBlock* FindBlock(int32_t y) const;

  void ScrollIfNeeded(bool x = true, bool y = true);
  void SetScrollPos(int32_t x, int32_t y);
  void SetLayoutSize(int32_t width, int32_t height, bool is_virtual_reformat);
  void Reformat(bool update_fragments);

  int32_t GetContentWidth();
  int32_t GetContentHeight() const;

  int32_t GetOverflowX() const {
    return std::max(content_width - layout_width, 0);
  }
  int32_t GetOverflowY() const {
    return std::max(content_height - layout_height, 0);
  }

 public:
  StyleEditListener* listener = nullptr;
  TextFragmentContentFactory default_content_factory;
  TextFragmentContentFactory* content_factory = &default_content_factory;
  int32_t layout_width = 0;
  int32_t layout_height = 0;
  int32_t content_width = 0;
  int32_t content_height = 0;

  TBLinkListOf<TextBlock> blocks;

  Caret caret = Caret(nullptr);
  TextSelection selection = TextSelection(nullptr);
  UndoRedoStack undoredo;

  int32_t scroll_x = 0;
  int32_t scroll_y = 0;

  int8_t select_state = 0;
  Point mousedown_point;
  TextFragment* mousedown_fragment = nullptr;

  /** DEPRECATED! This will be removed when using different fonts is properly
   * supported! */
  FontFace* font = nullptr;
  FontDescription font_desc;

  TextAlign align = TextAlign::kLeft;
  union {
    struct {
      uint32_t multiline_on : 1;
      uint32_t styling_on : 1;
      uint32_t read_only : 1;
      uint32_t selection_on : 1;
      uint32_t show_whitespace : 1;
      uint32_t password_on : 1;
      uint32_t wrapping : 1;
      uint32_t win_style_br : 1;
      // Whether content_width needs to be updated next GetContentWidth.
      uint32_t calculate_content_width_needed : 1;
      // Incremental counter for if UpdateScrollbar should be probhited.
      uint32_t lock_scrollbars_counter : 5;
    } packed;
    uint32_t packed_init = 0;
  };

  // Call BeginLockScrollbars & EndLockScrollbars around a scope which does lots
  // of changes, to prevent UpdateScrollbar from happening for each block. May
  // cause recalculation of content_width by iterating through all blocks.
  void BeginLockScrollbars();
  void EndLockScrollbars();

  // Returns true if changing layout_width and layout_height requires
  // relayouting.
  bool GetSizeAffectsLayout() const;
};

}  // namespace tb

#endif  // TB_STYLE_EDIT_H
