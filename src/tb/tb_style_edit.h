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

class TBStyleEdit;
class TBBlock;
class TBTextFragment;
class TBTextFragmentContent;
class TBTextFragmentContentFactory;

/** Listener for TBStyleEdit. Implement in the enviorment the TBStyleEdit should
 * render its content. */

class TBStyleEditListener {
 public:
  virtual ~TBStyleEditListener() {}

  virtual void OnChange(){};
  virtual bool OnEnter() { return false; };
  virtual void Invalidate(const TBRect& rect) = 0;
  virtual void DrawString(int32_t x, int32_t y, TBFontFace* font,
                          const TBColor& color, const char* str,
                          size_t len = std::string::npos) = 0;
  virtual void DrawRect(const TBRect& rect, const TBColor& color) = 0;
  virtual void DrawRectFill(const TBRect& rect, const TBColor& color) = 0;
  virtual void DrawTextSelectionBg(const TBRect& rect) = 0;
  virtual void DrawContentSelectionFg(const TBRect& rect) = 0;
  virtual void DrawCaret(const TBRect& rect) = 0;
  virtual void Scroll(int32_t dx, int32_t dy) = 0;
  virtual void UpdateScrollbars() = 0;
  virtual void CaretBlinkStart() = 0;
  virtual void CaretBlinkStop() = 0;
  virtual void OnBreak() {}
};

/** Creates TBTextFragmentContent if the sequence of text matches known content.
 */

class TBTextFragmentContentFactory {
 public:
  virtual ~TBTextFragmentContentFactory() {}
  /** Should return then length of the text that represents content
          that can be created by this factory, or 0 there's no match with any
     content.

          F.ex if we can create contet for "<u>" it should return 3 if that is
     the beginning of
          text. That length will be consumed from the text output for the
     created content. */
  virtual int GetContent(const char* text);

  /** Create content for a string previosly consumed by calling GetContent. */
  virtual TBTextFragmentContent* CreateFragmentContent(const char* text,
                                                       int text_len);
};

class TBTextOfs {
 public:
  TBTextOfs() : block(nullptr), ofs(0) {}
  TBTextOfs(TBBlock* block, size_t ofs) : block(block), ofs(ofs) {}

  void Set(TBBlock* new_block, size_t new_ofs) {
    block = new_block;
    ofs = new_ofs;
  }
  void Set(const TBTextOfs& pos) {
    block = pos.block;
    ofs = pos.ofs;
  }

  size_t GetGlobalOfs(TBStyleEdit* se) const;
  bool SetGlobalOfs(TBStyleEdit* se, size_t gofs);

 public:
  TBBlock* block;
  size_t ofs;
};

/** Handles the selected text in a TBStyleEdit. */

class TBSelection {
 public:
  TBSelection(TBStyleEdit* styledit);
  void Invalidate() const;
  void Select(const TBTextOfs& new_start, const TBTextOfs& new_stop);
  void Select(const TBPoint& from, const TBPoint& to);
  void Select(size_t glob_ofs_from, size_t glob_ofs_to);
  void SelectToCaret(TBBlock* old_caret_block, size_t old_caret_ofs);
  void SelectAll();
  void SelectNothing();
  void CorrectOrder();
  void CopyToClipboard();
  bool IsBlockSelected(TBBlock* block) const;
  bool IsFragmentSelected(TBTextFragment* elm) const;
  bool IsSelected() const;
  void RemoveContent();
  std::string GetText() const;

 public:
  TBStyleEdit* styledit;
  TBTextOfs start, stop;
};

enum class CaretPosition {
  kBeginning,
  kEnd,
};

// The caret in a TBStyleEdit.
class TBCaret {
 public:
  TBCaret(TBStyleEdit* styledit);
  void Invalidate();
  void UpdatePos();
  bool Move(bool forward, bool word);
  bool Place(const TBPoint& point);
  bool Place(TBBlock* block, size_t ofs, bool allow_snap = true,
             bool snap_forward = false);
  void Place(CaretPosition place);
  void AvoidLineBreak();
  void Paint(int32_t translate_x, int32_t translate_y);
  void ResetBlink();
  void UpdateWantedX();

  size_t GetGlobalOfs() const { return pos.GetGlobalOfs(styledit); }
  void SetGlobalOfs(size_t gofs, bool allow_snap = true,
                    bool snap_forward = false);

  TBTextFragment* GetFragment();

 private:
  void SwitchBlock(bool second);

 public:
  TBStyleEdit* styledit;
  int32_t x, y;  ///< Relative to the styledit
  int32_t width;
  int32_t height;
  int32_t wanted_x;
  bool on;
  bool prefer_first;
  TBTextOfs pos;
};

/** TBTextProps is a stack of properties used during layout & paint of
 * TBStyleEdit. */

class TBTextProps {
 public:
  class Data : public TBLinkOf<Data> {
   public:
    TBFontDescription font_desc;
    TBColor text_color;
    bool underline;
  };
  TBTextProps(const TBFontDescription& font_desc, const TBColor& text_color);

  Data* Push();
  void Pop();

  /** Get the font face from the current font description. */
  TBFontFace* GetFont();

 public:
  TBLinkListOf<Data> data_list;
  Data base_data;
  Data* data;
};

/** A block of text (a line, that might be wrapped) */

class TBBlock : public TBLinkOf<TBBlock> {
 public:
  TBBlock(TBStyleEdit* styledit);
  ~TBBlock();

  void Clear();
  void Set(const char* newstr, size_t len);
  void SetAlign(TextAlign align);

  size_t InsertText(size_t ofs, const char* text, size_t len,
                    bool allow_line_recurse);
  void RemoveContent(size_t ofs, size_t len);

  /** Check if this block contains extra line breaks and split into new blocks
   * if it does. */
  void Split();

  /** Check if we've lost the ending break on this block and if so merge it with
   * the next block. */
  void Merge();

  /** Layout the block. To be called when the text has changed or the layout
     width has changed.
          @param update_fragments Should be true if the text has been changed
     (will recreate elements).
          @param propagate_height If true, all following blocks will be moved if
     the height changed. */
  void Layout(bool update_fragments, bool propagate_height);

  /** Update the size of this block. If propagate_height is true, all following
     blocks will be
          moved if the height changed. */
  void SetSize(int32_t old_w, int32_t new_w, int32_t new_h,
               bool propagate_height);

  TBTextFragment* FindFragment(size_t ofs, bool prefer_first = false) const;
  TBTextFragment* FindFragment(int32_t x, int32_t y) const;

  int32_t CalculateStringWidth(TBFontFace* font, const char* str,
                               size_t len = std::string::npos) const;
  int32_t CalculateTabWidth(TBFontFace* font, int32_t xpos) const;
  int32_t CalculateLineHeight(TBFontFace* font) const;
  int32_t CalculateBaseline(TBFontFace* font) const;

  void Invalidate();
  void BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                            TBTextProps* props, TBRegion& bg_region,
                            TBRegion& fg_region);
  void Paint(int32_t translate_x, int32_t translate_y, TBTextProps* props);

 public:
  TBStyleEdit* styledit;
  TBLinkListOf<TBTextFragment> fragments;

  int32_t ypos;
  int16_t height;
  int8_t align;
  int line_width_max;

  std::string str;
  size_t str_len;

 private:
  int GetStartIndentation(TBFontFace* font, size_t first_line_len) const;
};

/** Event in the TBUndoRedoStack. Each insert or remove change is stored as a
 * TBUndoEvent, but they may also be merged when appropriate. */

class TBUndoEvent {
 public:
  size_t gofs;
  std::string text;
  bool insert;
};

/** Keeps track of all TBUndoEvents used for undo and redo functionality. */

class TBUndoRedoStack {
 public:
  TBUndoRedoStack() : applying(false) {}
  ~TBUndoRedoStack();

  void Undo(TBStyleEdit* styledit);
  void Redo(TBStyleEdit* styledit);
  void Clear(bool clear_undo, bool clear_redo);

  TBUndoEvent* Commit(TBStyleEdit* styledit, size_t gofs, size_t len,
                      const char* text, bool insert);

 public:
  TBListOf<TBUndoEvent> undos;
  TBListOf<TBUndoEvent> redos;
  bool applying;

 private:
  void Apply(TBStyleEdit* styledit, TBUndoEvent* e, bool reverse);
};

/** The textfragment baseclass for TBStyleEdit.

        TODO: This object is allocated on vast amounts and need
                  to shrink in size. Remove all cached positioning
                  and implement a fragment traverser (for TBBlock).
                  Also allocate fragments in chunks. */

class TBTextFragment : public TBLinkOf<TBTextFragment> {
 public:
  TBTextFragment(TBTextFragmentContent* content = nullptr)
      : xpos(0),
        ypos(0),
        ofs(0),
        len(0),
        line_ypos(0),
        line_height(0),
        block(nullptr),
        content(content) {}
  ~TBTextFragment();

  void Init(TBBlock* block, uint16_t ofs, uint16_t len);

  void UpdateContentPos();

  void BuildSelectionRegion(int32_t translate_x, int32_t translate_y,
                            TBTextProps* props, TBRegion& bg_region,
                            TBRegion& fg_region);
  void Paint(int32_t translate_x, int32_t translate_y, TBTextProps* props);
  void Click(int button, ModifierKeys modifierkeys);

  bool IsText() const { return !IsEmbedded(); }
  bool IsEmbedded() const { return content ? true : false; }
  bool IsBreak() const;
  bool IsSpace() const;
  bool IsTab() const;

  int32_t GetCharX(TBFontFace* font, size_t ofs);
  size_t GetCharOfs(TBFontFace* font, int32_t x);

  /** Get the stringwidth. Handles passwordmode, tab, linebreaks etc
   * automatically. */
  int32_t GetStringWidth(TBFontFace* font, const char* str, size_t len);

  bool GetAllowBreakBefore() const;
  bool GetAllowBreakAfter() const;

  const char* Str() const { return block->str.c_str() + ofs; }

  int32_t GetWidth(TBFontFace* font);
  int32_t GetHeight(TBFontFace* font);
  int32_t GetBaseline(TBFontFace* font);

 public:
  int16_t xpos, ypos;
  uint16_t ofs, len;
  uint16_t line_ypos;
  uint16_t line_height;
  TBBlock* block;
  TBTextFragmentContent* content;
};

/** Edit and formats TBTextFragment's. It handles the text in a TBStyleEditView.
 */

class TBStyleEdit {
 public:
  TBStyleEdit();
  virtual ~TBStyleEdit();

  void SetListener(TBStyleEditListener* listener);
  void SetContentFactory(TBTextFragmentContentFactory* content_factory);

  void SetFont(const TBFontDescription& font_desc);

  void Paint(const TBRect& rect, const TBFontDescription& font_desc,
             const TBColor& text_color);
  bool KeyDown(int key, SpecialKey special_key, ModifierKeys modifierkeys);
  bool MouseDown(const TBPoint& point, int button, int clicks,
                 ModifierKeys modifierkeys, bool touch);
  bool MouseUp(const TBPoint& point, int button, ModifierKeys modifierkeys,
               bool touch);
  bool MouseMove(const TBPoint& point);
  void Focus(bool focus);

  void Clear(bool init_new = true);
  bool Load(const char* filename);
  void SetText(const char* text, CaretPosition pos = CaretPosition::kBeginning);
  void SetText(const char* text, size_t text_len,
               CaretPosition pos = CaretPosition::kBeginning);
  std::string GetText();
  bool IsEmpty() const;

  /** Set the default text alignment and all currently selected blocks,
          or the block of the current caret position if nothing is selected. */
  void SetAlign(TextAlign align);
  void SetMultiline(bool multiline = true);
  void SetStyling(bool styling = true);
  void SetReadOnly(bool readonly = true);
  void SetSelection(bool selection = true);
  void SetPassword(bool password = true);
  void SetWrapping(bool wrapping = true);

  /** Set if line breaks should be inserted in windows style (\r\n)
          or unix style (\n). The default is windows style on the windows
          platform and disabled elsewhere.

          Note: This only affects InsertBreak (pressing enter). Content set from
                SetText (and clipboard etc.) maintains the used line break. */
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

  TBBlock* FindBlock(int32_t y) const;

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
  TBStyleEditListener* listener;
  TBTextFragmentContentFactory default_content_factory;
  TBTextFragmentContentFactory* content_factory;
  int32_t layout_width;
  int32_t layout_height;
  int32_t content_width;
  int32_t content_height;

  TBLinkListOf<TBBlock> blocks;

  TBCaret caret;
  TBSelection selection;
  TBUndoRedoStack undoredo;

  int32_t scroll_x;
  int32_t scroll_y;

  int8_t select_state;
  TBPoint mousedown_point;
  TBTextFragment* mousedown_fragment;

  /** DEPRECATED! This will be removed when using different fonts is properly
   * supported! */
  TBFontFace* font;
  TBFontDescription font_desc;

  TextAlign align;
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
      uint32_t
          calculate_content_width_needed : 1;  ///< If content_width needs to
      /// be updated next
      /// GetContentWidth-
      uint32_t lock_scrollbars_counter : 5;  ///< Incremental counter for if
                                             /// UpdateScrollbar should be
                                             /// probhited.
    } packed;
    uint32_t packed_init;
  };

  /** Call BeginLockScrollbars & EndLockScrollbars around a scope which does
     lots of changes,
          to prevent UpdateScrollbar from happening for each block (May cause
     recalculation of
          content_width by iterating through all blocks) */
  void BeginLockScrollbars();
  void EndLockScrollbars();

  /** Return true if changing layout_width and layout_height requires
   * relayouting. */
  bool GetSizeAffectsLayout() const;
};

}  // namespace tb

#endif  // TB_STYLE_EDIT_H
