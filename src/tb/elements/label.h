/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_LABEL_H_
#define TB_ELEMENTS_LABEL_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// ElementString holds a string that can be painted as one line with the set
// alignment.
class ElementString {
 public:
  ElementString();

  void Paint(Element* element, const Rect& rect, const Color& color);

  int GetWidth(Element* element);
  int GetHeight(Element* element);

  void SetText(const char* text) { m_text = text; }
  std::string GetText() const { return m_text; }

  bool empty() const { return m_text.empty(); }

  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  void SetTextAlign(TextAlign align) { m_text_align = align; }
  TextAlign GetTextAlign() { return m_text_align; }

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

  // Sets the text of the text field.
  void SetText(const char* text) override;
  using Element::SetText;
  std::string GetText() override { return m_text.GetText(); }

  bool empty() const { return m_text.empty(); }

  // Sets which alignment the text should have if the space given when painting
  // is larger than the text.
  void SetTextAlign(TextAlign align) { m_text.SetTextAlign(align); }
  TextAlign GetTextAlign() { return m_text.GetTextAlign(); }

  // Sets if this text field should be allowed to squeeze below its preferred
  // size. If squeezable it may shrink to width 0.
  void SetSqueezable(bool squeezable);
  bool GetSqueezable() { return m_squeezable; }

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
}  // namespace tb

#endif  // TB_ELEMENTS_LABEL_H_
