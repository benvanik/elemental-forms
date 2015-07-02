/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_EDITFIELD_H
#define TB_EDITFIELD_H

#include "tb_msg.h"
#include "tb_style_edit.h"
#include "tb_widgets_common.h"

namespace tb {

// These types does not restrict input (may change in the future). They are just
// hints for virtual keyboard, so it can show special keys.
enum class EditType {
  kText,
  kSearch,
  kPassword,
  kEmail,
  kPhone,
  kUrl,
  kNumber
};
MAKE_ORDERED_ENUM_STRING_UTILS(EditType, "text", "search", "password", "email",
                               "phone", "url", "number");

/** The default content factory for embedded content in TBEditField with styling
   enabled.

        Creates all that TBTextFragmentContentFactory creates by default,
        and any type of widget from a inline resource string.

        Syntax: <widget xxx> Where xxx is parsed by TBWidgetsReader.

        Example - Create a button with id "hello":

                <widget Button: text: "Hello world!" id: "hello">

        Example - Create a image from skin element "Icon48":

                <widget TBSkinImage: skin: "Icon48">
*/

class TBEditFieldContentFactory : public TBTextFragmentContentFactory {
 public:
  class TBEditField* editfield;
  virtual int GetContent(const char* text);
  virtual TBTextFragmentContent* CreateFragmentContent(const char* text,
                                                       int text_len);
};

/** TBEditFieldScrollRoot - Internal for TBEditField.
        Acts as a scrollable container for any widget created as embedded
   content. */

class TBEditFieldScrollRoot : public TBWidget {
 private:  // May only be used by TBEditField.
  friend class TBEditField;
  TBEditFieldScrollRoot() {}

 public:
  virtual void OnPaintChildren(const PaintProps& paint_props);
  virtual void GetChildTranslation(int& x, int& y) const;
  virtual HitStatus GetHitStatus(int x, int y);
};

/** TBEditField is a one line or multi line textfield that is editable or
        read-only. It can also be a passwordfield by calling
        SetEditType(EditType::kPassword).

        It may perform styling of text and contain custom embedded content,
        if enabled by SetStyling(true). Disabled by default.
*/

class TBEditField : public TBWidget,
                    private TBStyleEditListener,
                    public MessageHandler {
 public:
  TBOBJECT_SUBCLASS(TBEditField, TBWidget);

  TBEditField();
  ~TBEditField();

  /** Get the visible rect (the GetPaddingRect() minus any scrollbars) */
  Rect GetVisibleRect();

  /** Set if multiple lines should be allowed or not.
          Will also set wrapping (to true if multiline, and false if not). */
  void SetMultiline(bool multiline);
  bool GetMultiline() const { return m_style_edit.packed.multiline_on; }

  /** Set if styling should be enabled or not. Default is disabled. */
  void SetStyling(bool styling);
  bool GetStyling() const { return m_style_edit.packed.styling_on; }

  /** Set if read only mode should be enabled. Default is disabled.
          In read only mode, editing is disabled and caret is hidden.
          The user is still able to focus, select and copy text. */
  void SetReadOnly(bool readonly);
  bool GetReadOnly() const { return m_style_edit.packed.read_only; }

  /** Set to true if the text should wrap if multi line is enabled (See
   * SetMultiline). */
  void SetWrapping(bool wrapping);
  bool GetWrapping() const { return m_style_edit.packed.wrapping; }

  /** Set to true if the preferred size of this editfield should adapt to the
          size of the content (disabled by default).
          If wrapping is enabled, the result is partly dependant on the virtual
          width (See SetVirtualWidth). */
  void SetAdaptToContentSize(bool adapt);
  bool GetAdaptToContentSize() const { return m_adapt_to_content_size; }

  /** The virtual width is only used if the size is adapting to content size
          (See SetAdaptToContentSize) and wrapping is enabled.
          The virtual width will be used to layout the text and see which
     resulting
          width and height it takes up. The width that is actually used depends
     on
          the content. It is also up to the the layouter to decide if the size
          should be respected or not. The default is 250. */
  void SetVirtualWidth(int virtual_width);
  int GetVirtualWidth() const { return m_virtual_width; }

  /** Get the TBStyleEdit object that contains more functions and settings. */
  TBStyleEdit* GetStyleEdit() { return &m_style_edit; }

  /** Set the edit type that is a hint for virtual keyboards about what the
          content should be. */
  void SetEditType(EditType type);
  EditType GetEditType() { return m_edit_type; }

  /** Support custom skin condition properties. Currently supported properties
     are:
          "edit-type", matching those of EditType.
          "multiline", matching 1 if multiline mode is enabled.
          "readonly", matching 1 if readonly mode is enabled. */
  virtual bool GetCustomSkinCondition(
      const TBSkinCondition::ConditionInfo& info);

  /** Set which alignment the text should have if the space
          given when painting is larger than the text.
          This changes the default for new blocks, as wel as the currently
     selected blocks or the block
          of the current caret position if nothing is selected. */
  void SetTextAlign(TextAlign align) { m_style_edit.SetAlign(align); }
  TextAlign GetTextAlign() { return m_style_edit.align; }

  void SetText(const char* text) override {
    m_style_edit.SetText(text, CaretPosition::kBeginning);
  }
  using TBWidget::SetText;
  std::string GetText() override { return m_style_edit.GetText(); }

  using TBWidget::Invalidate;  ///< Make Invalidate in base class available.

  /** Set the text and also specify if the caret should be positioned at the
     beginning
          or end of the text. */
  void SetText(const char* text, CaretPosition pos) {
    m_style_edit.SetText(text, pos);
  }
  /** Set the text of the given length and also specify if the caret should be
     positioned
          at the beginning or end of the text. */
  void SetText(const char* text, size_t text_len,
               CaretPosition pos = CaretPosition::kBeginning) {
    m_style_edit.SetText(text, text_len, pos);
  }

  /** Set the placeholder text. It will be visible only when the textfield is
   * empty. */
  virtual void SetPlaceholderText(const char* text) {
    m_placeholder.SetText(text);
  }
  virtual std::string GetPlaceholderText() { return m_placeholder.GetText(); }

  virtual void ScrollTo(int x, int y);
  virtual TBWidget::ScrollInfo GetScrollInfo();
  virtual TBWidget* GetScrollRoot() { return &m_root; }

  virtual bool OnEvent(const TBWidgetEvent& ev);
  virtual void OnPaint(const PaintProps& paint_props);
  virtual void OnPaintChildren(const PaintProps& paint_props);
  virtual void OnInflate(const INFLATE_INFO& info);
  virtual void OnAdded();
  virtual void OnFontChanged();
  virtual void OnFocusChanged(bool focused);
  virtual void OnResized(int old_w, int old_h);
  virtual TBWidget* GetContentRoot() { return &m_root; }

  virtual PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints);

  virtual void OnMessageReceived(Message* msg);

 protected:
  ScrollBar m_scrollbar_x;
  ScrollBar m_scrollbar_y;
  WidgetString m_placeholder;
  EditType m_edit_type;
  TBEditFieldScrollRoot m_root;
  TBEditFieldContentFactory m_content_factory;
  TBStyleEdit m_style_edit;
  bool m_adapt_to_content_size;
  int m_virtual_width;
  void UpdateScrollbarVisibility(bool multiline);

  // == TBStyleEditListener =======================
  virtual void OnChange();
  virtual bool OnEnter();
  virtual void Invalidate(const Rect& rect);
  virtual void DrawString(int32_t x, int32_t y, FontFace* font,
                          const Color& color, const char* str, size_t len);
  virtual void DrawRect(const Rect& rect, const Color& color);
  virtual void DrawRectFill(const Rect& rect, const Color& color);
  virtual void DrawTextSelectionBg(const Rect& rect);
  virtual void DrawContentSelectionFg(const Rect& rect);
  virtual void DrawCaret(const Rect& rect);
  virtual void Scroll(int32_t dx, int32_t dy);
  virtual void UpdateScrollbars();
  virtual void CaretBlinkStart();
  virtual void CaretBlinkStop();
};

}  // namespace tb

#endif  // TB_EDITFIELD_H
