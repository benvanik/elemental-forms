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

  // Sets along which axis the content should layout (if the label has more
  // content than the text).
  void SetAxis(Axis axis) override { m_layout.SetAxis(axis); }
  Axis GetAxis() const override { return m_layout.GetAxis(); }

  // Sets the text of the label.
  void SetText(const char* text) override { m_textfield.SetText(text); }
  std::string GetText() override { return m_textfield.GetText(); }

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override {
    return m_layout.GetPreferredSize();
  }

  Element* GetContentRoot() override { return &m_layout; }

  bool OnEvent(const ElementEvent& ev) override;

 protected:
  LayoutBox m_layout;
  Label m_textfield;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_LABEL_CONTAINER_H_
