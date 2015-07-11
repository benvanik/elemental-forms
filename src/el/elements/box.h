/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_BOX_H_
#define EL_ELEMENTS_BOX_H_

#include "el/element.h"

namespace el {
namespace elements {

// Box is just a Element with border and padding (using skin
// "Box").
class Box : public Element {
 public:
  TBOBJECT_SUBCLASS(Box, Element);
  static void RegisterInflater();

  Box();
};

}  // namespace elements
namespace dsl {

struct BoxNode : public ElementNode<BoxNode> {
  using R = BoxNode;
  BoxNode() : ElementNode("Box") {}
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_BOX_H_
