/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_EDITFIELD_H
#define TB_EDITFIELD_H

#include "tb_msg.h"
#include "tb_style_edit.h"
#include "tb_widgets_common.h"

#include "tb/types.h"

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

// The default content factory for embedded content in TextBox with styling
// enabled.
// Creates all that TextFragmentContentFactory creates by default, and any type
// of element from a inline resource string.
//
// Syntax: <element xxx>
// (where xxx is parsed by ElementFactory).
//
// Example - Create a button with id "hello":
//   <element Button: text: "Hello world!" id: "hello">
// Example - Create a image from skin element "Icon48":
//   <element SkinImage: skin: "Icon48">
class TextBoxContentFactory : public TextFragmentContentFactory {
 public:
  class TextBox* text_box = nullptr;
  int GetContent(const char* text) override;
  TextFragmentContent* CreateFragmentContent(const char* text,
                                             size_t text_len) override;
};

// Internal for TextBox.
// Acts as a scrollable container for any element created as embedded content.
class TextBoxScrollRoot : public Element {
 private:
  // May only be used by TextBox.
  friend class TextBox;
  TextBoxScrollRoot() = default;

 public:
  void OnPaintChildren(const PaintProps& paint_props) override;
  void GetChildTranslation(int& x, int& y) const override;
  HitStatus GetHitStatus(int x, int y) override;
};

// A one line or multi line textfield that is editable or read-only.
// It can also be a passwordfield by calling SetEditType(EditType::kPassword).
// It may perform styling of text and contain custom embedded content, if
// enabled by SetStyling(true). Disabled by default.
class TextBox : public Element,
                private StyleEditListener,
                public MessageHandler {
 public:
  TBOBJECT_SUBCLASS(TextBox, Element);
  static void RegisterInflater();

  TextBox();
  ~TextBox() override;

  // Get the visible rect (the GetPaddingRect() minus any scrollbars).
  Rect GetVisibleRect();

  // Set if multiple lines should be allowed or not. Will also set wrapping (to
  // true if multiline, and false if not).
  void SetMultiline(bool multiline);
  bool GetMultiline() const { return m_style_edit.packed.multiline_on; }

  // Sets if styling should be enabled or not. Default is disabled.
  void SetStyling(bool styling);
  bool GetStyling() const { return m_style_edit.packed.styling_on; }

  // Sets if read only mode should be enabled. Default is disabled.
  // In read only mode, editing is disabled and caret is hidden. The user is
  // still able to focus, select and copy text.
  void SetReadOnly(bool readonly);
  bool GetReadOnly() const { return m_style_edit.packed.read_only; }

  // Sets if the text should wrap if multi line is enabled (see SetMultiline).
  void SetWrapping(bool wrapping);
  bool GetWrapping() const { return m_style_edit.packed.wrapping; }

  // Sets to true if the preferred size of this text_box should adapt to the
  // size of the content (disabled by default).
  // If wrapping is enabled, the result is partly dependant on the virtual width
  // (see SetVirtualWidth).
  void SetAdaptToContentSize(bool adapt);
  bool GetAdaptToContentSize() const { return m_adapt_to_content_size; }

  // The virtual width is only used if the size is adapting to content size (see
  // SetAdaptToContentSize) and wrapping is enabled.
  // The virtual width will be used to layout the text and see which resulting
  // width and height it takes up. The width that is actually used depends on
  // the content. It is also up to the layouter to decide if the size should
  // be respected or not. The default is 250.
  void SetVirtualWidth(int virtual_width);
  int GetVirtualWidth() const { return m_virtual_width; }

  // Gets the StyleEdit object that contains more functions and settings.
  StyleEdit* GetStyleEdit() { return &m_style_edit; }

  // Sets the edit type that is a hint for virtual keyboards about what the
  // content should be.
  void SetEditType(EditType type);
  EditType GetEditType() { return m_edit_type; }

  // Supports custom skin condition properties. Currently supported properties
  // are:
  //     "edit-type", matching those of EditType.
  //     "multiline", matching 1 if multiline mode is enabled.
  //     "readonly", matching 1 if readonly mode is enabled.
  bool GetCustomSkinCondition(
      const SkinCondition::ConditionInfo& info) override;

  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  // This changes the default for new blocks, as wel as the currently selected
  // blocks or the block of the current caret position if nothing is selected.
  void SetTextAlign(TextAlign align) { m_style_edit.SetAlign(align); }
  TextAlign GetTextAlign() { return m_style_edit.align; }

  void SetText(const char* text) override {
    m_style_edit.SetText(text, CaretPosition::kBeginning);
  }
  using Element::SetText;
  std::string GetText() override { return m_style_edit.GetText(); }

  // Sets the text and also specify if the caret should be positioned at the
  // beginning or end of the text.
  void SetText(const char* text, CaretPosition pos) {
    m_style_edit.SetText(text, pos);
  }
  // Sets the text of the given length and also specify if the caret should be
  // positioned at the beginning or end of the text.
  void SetText(const char* text, size_t text_len,
               CaretPosition pos = CaretPosition::kBeginning) {
    m_style_edit.SetText(text, text_len, pos);
  }

  using Element::Invalidate;

  // Set the placeholder text. It will be visible only when the textfield is
  // empty.
  virtual void SetPlaceholderText(const char* text) {
    m_placeholder.SetText(text);
  }
  virtual std::string GetPlaceholderText() { return m_placeholder.GetText(); }

  void ScrollTo(int x, int y) override;
  Element::ScrollInfo GetScrollInfo() override;
  Element* GetScrollRoot() override { return &m_root; }

  bool OnEvent(const ElementEvent& ev) override;
  void OnPaint(const PaintProps& paint_props) override;
  void OnPaintChildren(const PaintProps& paint_props) override;
  void OnInflate(const resources::InflateInfo& info) override;
  void OnAdded() override;
  void OnFontChanged() override;
  void OnFocusChanged(bool focused) override;
  void OnResized(int old_w, int old_h) override;
  Element* GetContentRoot() override { return &m_root; }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnMessageReceived(Message* msg) override;

 protected:
  void UpdateScrollbarVisibility(bool multiline);

  void OnChange() override;
  bool OnEnter() override;
  void Invalidate(const Rect& rect) override;
  void DrawString(int32_t x, int32_t y, resources::FontFace* font,
                  const Color& color, const char* str, size_t len) override;
  void DrawRect(const Rect& rect, const Color& color) override;
  void DrawRectFill(const Rect& rect, const Color& color) override;
  void DrawTextSelectionBg(const Rect& rect) override;
  void DrawContentSelectionFg(const Rect& rect) override;
  void DrawCaret(const Rect& rect) override;
  void Scroll(int32_t dx, int32_t dy) override;
  void UpdateScrollbars() override;
  void CaretBlinkStart() override;
  void CaretBlinkStop() override;

  ScrollBar m_scrollbar_x;
  ScrollBar m_scrollbar_y;
  ElementString m_placeholder;
  EditType m_edit_type = EditType::kText;
  TextBoxScrollRoot m_root;
  TextBoxContentFactory m_content_factory;
  StyleEdit m_style_edit;
  bool m_adapt_to_content_size = false;
  int m_virtual_width = 250;
};

}  // namespace tb

#endif  // TB_EDITFIELD_H
