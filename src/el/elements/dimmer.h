/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_DIMMER_H_
#define EL_ELEMENTS_DIMMER_H_

#include "el/element.h"

namespace el {
namespace elements {

// Dims elements in the background and blocks input.
class Dimmer : public Element {
 public:
  TBOBJECT_SUBCLASS(Dimmer, Element);
  static void RegisterInflater();

  Dimmer();

  void OnAdded() override;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_DIMMER_H_
