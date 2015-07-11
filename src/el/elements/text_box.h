/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_EDITFIELD_H
#define EL_EDITFIELD_H

#include "el/element.h"
#include "el/elements/label.h"
#include "el/elements/scroll_bar.h"
#include "el/message_handler.h"
#include "el/text/text_view.h"
#include "el/types.h"

namespace el {
namespace elements {

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

// A one line or multi line textfield that is editable or read-only.
// It can also be a passwordfield by calling set_edit_type(EditType::kPassword).
// It may perform styling of text and contain custom embedded content, if
// enabled by set_styled(true). Disabled by default.
class TextBox : public Element,
                private text::TextViewListener,
                public MessageHandler {
 public:
  TBOBJECT_SUBCLASS(TextBox, Element);
  static void RegisterInflater();

  TextBox();
  ~TextBox() override;

  // Get the visible rect (the padding_rect() minus any scrollbars).
  Rect visible_rect();

  bool is_multiline() const { return m_style_edit.packed.multiline_on; }
  // Set if multiple lines should be allowed or not. Will also set wrapping (to
  // true if multiline, and false if not).
  void set_multiline(bool multiline);

  bool is_styled() const { return m_style_edit.packed.styling_on; }
  // Sets if styling should be enabled or not. Default is disabled.
  void set_styled(bool styling);

  bool is_read_only() const { return m_style_edit.packed.read_only; }
  // Sets if read only mode should be enabled. Default is disabled.
  // In read only mode, editing is disabled and caret is hidden. The user is
  // still able to focus, select and copy text.
  void set_read_only(bool readonly);

  bool is_wrapping() const { return m_style_edit.packed.wrapping; }
  // Sets if the text should wrap if multi line is enabled (see set_multiline).
  void set_wrapping(bool wrapping);

  bool is_adapting_to_content_size() const { return m_adapt_to_content_size; }
  // Sets to true if the preferred size of this text_box should adapt to the
  // size of the content (disabled by default).
  // If wrapping is enabled, the result is partly dependant on the virtual width
  // (see set_virtual_width).
  void set_adapt_to_content_size(bool adapt);

  int virtual_width() const { return m_virtual_width; }
  // The virtual width is only used if the size is adapting to content size (see
  // set_adapt_to_content_size) and wrapping is enabled.
  // The virtual width will be used to layout the text and see which resulting
  // width and height it takes up. The width that is actually used depends on
  // the content. It is also up to the layouter to decide if the size should
  // be respected or not. The default is 250.
  void set_virtual_width(int virtual_width);

  // Gets the TextView object that contains more functions and settings.
  text::TextView* text_view() { return &m_style_edit; }

  EditType edit_type() { return m_edit_type; }
  // Sets the edit type that is a hint for virtual keyboards about what the
  // content should be.
  void set_edit_type(EditType type);

  // Supports custom skin condition properties. Currently supported properties
  // are:
  //     "edit-type", matching those of EditType.
  //     "multiline", matching 1 if multiline mode is enabled.
  //     "readonly", matching 1 if readonly mode is enabled.
  bool GetCustomSkinCondition(
      const el::SkinCondition::ConditionInfo& info) override;

  TextAlign text_align() { return m_style_edit.align; }
  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  // This changes the default for new blocks, as wel as the currently selected
  // blocks or the block of the current caret position if nothing is selected.
  void set_text_align(TextAlign align) { m_style_edit.set_alignment(align); }

  void set_text(const char* text) override {
    m_style_edit.set_text(text, text::CaretPosition::kBeginning);
  }
  std::string text() override { return m_style_edit.text(); }
  using Element::set_text;
  // Sets the text and also specify if the caret should be positioned at the
  // beginning or end of the text.
  void set_text(const char* text, text::CaretPosition pos) {
    m_style_edit.set_text(text, pos);
  }
  // Sets the text of the given length and also specify if the caret should be
  // positioned at the beginning or end of the text.
  void set_text(const char* text, size_t text_len,
                text::CaretPosition pos = text::CaretPosition::kBeginning) {
    m_style_edit.set_text(text, text_len, pos);
  }

  virtual std::string placeholder_text() { return m_placeholder.text(); }
  // Set the placeholder text. It will be visible only when the textfield is
  // empty.
  virtual void set_placeholder_text(const char* text) {
    m_placeholder.set_text(text);
  }

  using Element::Invalidate;

  void ScrollTo(int x, int y) override;
  Element::ScrollInfo scroll_info() override;
  Element* scroll_root() override { return &m_root; }

  bool OnEvent(const Event& ev) override;
  void OnPaint(const PaintProps& paint_props) override;
  void OnPaintChildren(const PaintProps& paint_props) override;
  void OnInflate(const parsing::InflateInfo& info) override;
  void OnAdded() override;
  void OnFontChanged() override;
  void OnFocusChanged(bool focused) override;
  void OnResized(int old_w, int old_h) override;
  Element* content_root() override { return &m_root; }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnMessageReceived(Message* msg) override;

 protected:
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

  // The default content factory for embedded content in TextBox with styling
  // enabled.
  // Creates all that TextFragmentContentFactory creates by default, and any
  // type
  // of element from a inline resource string.
  //
  // Syntax: <element xxx>
  // (where xxx is parsed by ElementFactory).
  //
  // Example - Create a button with id "hello":
  //   <element Button: text: "Hello world!" id: "hello">
  // Example - Create a image from skin element "Icon48":
  //   <element IconBox: skin: "Icon48">
  class TextBoxContentFactory : public text::TextFragmentContentFactory {
   public:
    class TextBox* text_box = nullptr;
    int GetContent(const char* text) override;
    text::TextFragmentContent* CreateFragmentContent(const char* text,
                                                     size_t text_len) override;
  };

  void UpdateScrollbarVisibility(bool multiline);

  void OnChange() override;
  bool OnEnter() override;
  void Invalidate(const Rect& rect) override;
  void DrawString(int32_t x, int32_t y, el::text::FontFace* font,
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
  text::TextView m_style_edit;
  bool m_adapt_to_content_size = false;
  int m_virtual_width = 250;
};

}  // namespace elements
namespace dsl {

using el::elements::EditType;

struct TextBoxNode : public ElementNode<TextBoxNode> {
  using R = TextBoxNode;
  TextBoxNode(const char* text = nullptr) : ElementNode("TextBox") {
    if (text) {
      this->text(text);
    }
  }
  //
  R& text(std::string value) {
    set("text", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_multiline(bool value) {
    set("multiline", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_styling(bool value) {
    set("styling", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_read_only(bool value) {
    set("readonly", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& is_wrapping(bool value) {
    set("wrap", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& adapt_to_content(bool value) {
    set("adapt-to-content", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& virtual_width(Dimension value) {
    set("virtual-width", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& placeholder(std::string value) {
    set("placeholder", value);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& type(EditType value) {
    set("type", el::elements::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_EDITFIELD_H
