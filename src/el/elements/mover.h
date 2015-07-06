/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_MOVER_H_
#define EL_ELEMENTS_MOVER_H_

#include "el/element.h"

namespace el {
namespace elements {

// Moves its parent element when dragged.
class Mover : public Element {
 public:
  TBOBJECT_SUBCLASS(Mover, Element);
  static void RegisterInflater();

  Mover();

  bool OnEvent(const Event& ev) override;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_MOVER_H_
