/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TEXT_TEXT_VIEW_H_
#define TB_TEXT_TEXT_VIEW_H_

#include <algorithm>
#include <memory>
#include <vector>

#include "tb/color.h"
#include "tb/font_description.h"
#include "tb/element.h"
#include "tb/text/caret.h"
#include "tb/text/text_fragment.h"
#include "tb/text/undo_stack.h"
#include "tb/util/link_list.h"
#include "tb/util/rect_region.h"

namespace tb {
namespace text {

class FontFace;
class TextView;
class TextBlock;
class TextFragment;
class TextFragmentContentFactory;

// Listener for TextView. Implement in the environment the TextView should
// render its content.
class TextViewListener {
 public:
  virtual ~TextViewListener() = default;

  virtual void OnChange() {}
  virtual bool OnEnter() { return false; }
  virtual void Invalidate(const Rect& rect) = 0;
  virtual void DrawString(int32_t x, int32_t y, text::FontFace* font,
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

// Edits and formats TextFragment's.
class TextView {
 public:
  TextView();
  virtual ~TextView();

  void SetListener(TextViewListener* listener);
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
  bool CanUndo() const { return !undo_stack.undos.empty(); }
  bool CanRedo() const { return !undo_stack.redos.empty(); }

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
  TextViewListener* listener = nullptr;
  TextFragmentContentFactory default_content_factory;
  TextFragmentContentFactory* content_factory = &default_content_factory;
  int32_t layout_width = 0;
  int32_t layout_height = 0;
  int32_t content_width = 0;
  int32_t content_height = 0;

  tb::util::TBLinkListOf<TextBlock> blocks;

  Caret caret = Caret(nullptr);
  TextSelection selection = TextSelection(nullptr);
  UndoStack undo_stack;

  int32_t scroll_x = 0;
  int32_t scroll_y = 0;

  int8_t select_state = 0;
  Point mousedown_point;
  TextFragment* mousedown_fragment = nullptr;

  /** DEPRECATED! This will be removed when using different fonts is properly
   * supported! */
  text::FontFace* font = nullptr;
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

}  // namespace text
}  // namespace tb

#endif  // TB_TEXT_TEXT_VIEW_H_
