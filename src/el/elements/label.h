/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_LABEL_H_
#define EL_ELEMENTS_LABEL_H_

#include "el/element.h"

namespace el {
namespace elements {

// ElementString holds a string that can be painted as one line with the set
// alignment.
class ElementString {
 public:
  ElementString();

  void Paint(Element* element, const Rect& rect, const Color& color);

  int GetWidth(Element* element);
  int GetHeight(Element* element);

  void set_text(const char* text) { m_text = text; }
  std::string text() const { return m_text; }

  bool empty() const { return m_text.empty(); }

  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  void set_text_align(TextAlign align) { m_text_align = align; }
  TextAlign text_align() { return m_text_align; }

 public:
  std::string m_text;
  TextAlign m_text_align = TextAlign::kCenter;
};

// A one line text field that is not editable.
class Label : public Element {
 public:
  TBOBJECT_SUBCLASS(Label, Element);
  static void RegisterInflater();

  Label();

  std::string text() override { return m_text.text(); }
  // Sets the text of the text field.
  void set_text(const char* text) override;
  using Element::set_text;

  bool empty() const { return m_text.empty(); }

  TextAlign text_align() { return m_text.text_align(); }
  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  void set_text_align(TextAlign align) { m_text.set_text_align(align); }

  bool is_squeezable() { return m_squeezable; }
  // Sets if this text field should be allowed to squeeze below its preferred
  // size. If squeezable it may shrink to width 0.
  void set_squeezable(bool squeezable);

  void OnInflate(const parsing::InflateInfo& info) override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;
  void OnFontChanged() override;
  void OnPaint(const PaintProps& paint_props) override;

 protected:
  // This value on m_cached_text_width means it needs to be updated again.
  static const int kTextWidthCacheNeedsUpdate = -1;

  ElementString m_text;
  int m_cached_text_width = kTextWidthCacheNeedsUpdate;
  bool m_squeezable = false;
};

}  // namespace elements
namespace dsl {

struct LabelNode : public ElementNode<LabelNode> {
  using R = LabelNode;
  LabelNode(const char* text = nullptr) : ElementNode("Label") {
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
  R& text_align(TextAlign value) {
    set("text-align", el::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_LABEL_H_
