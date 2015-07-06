/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_SEPARATOR_H_
#define EL_ELEMENTS_SEPARATOR_H_

#include "el/element.h"

namespace el {
namespace elements {

// A element only showing a skin.
// It is disabled by default.
class Separator : public Element {
 public:
  TBOBJECT_SUBCLASS(Separator, Element);
  static void RegisterInflater();

  Separator();
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_SEPARATOR_H_
