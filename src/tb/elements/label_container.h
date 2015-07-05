/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_LABEL_CONTAINER_H_
#define TB_ELEMENTS_LABEL_CONTAINER_H_

#include "tb/element.h"
#include "tb/elements/label.h"
#include "tb/elements/layout_box.h"

namespace tb {
namespace elements {

// Has a text field in its internal layout by default. Pointer input on the text
// field will be redirected to another child element (that you add) to it.
// Typically useful for creating check boxes, radio buttons with labels.
class LabelContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(LabelContainer, Element);
  static void RegisterInflater();

  LabelContainer();
  ~LabelContainer() override;

  Axis axis() const override { return m_layout.axis(); }
  // Sets along which axis the content should layout (if the label has more
  // content than the text).
  void set_axis(Axis axis) override { m_layout.set_axis(axis); }

  std::string text() override { return m_textfield.text(); }
  // Sets the text of the label.
  void set_text(const char* text) override { m_textfield.set_text(text); }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override {
    return m_layout.GetPreferredSize();
  }

  Element* content_root() override { return &m_layout; }

  bool OnEvent(const ElementEvent& ev) override;

 protected:
  LayoutBox m_layout;
  Label m_textfield;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_LABEL_CONTAINER_H_
