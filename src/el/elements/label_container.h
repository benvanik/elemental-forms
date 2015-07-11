/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_LABEL_CONTAINER_H_
#define EL_ELEMENTS_LABEL_CONTAINER_H_

#include "el/element.h"
#include "el/elements/label.h"
#include "el/elements/layout_box.h"

namespace el {
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

  bool OnEvent(const Event& ev) override;

 protected:
  LayoutBox m_layout;
  Label m_textfield;
};

}  // namespace elements
namespace dsl {

struct LabelContainerNode : public ElementNode<LabelContainerNode> {
  using R = LabelContainerNode;
  LabelContainerNode(const char* text, std::vector<Node> children = {})
      : ElementNode("LabelContainer", {}, std::move(children)) {
    if (text) {
      this->text(text);
    }
  }
  //
  R& text(std::string value) {
    set("text", value);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_LABEL_CONTAINER_H_
