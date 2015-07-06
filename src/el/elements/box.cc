/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/box.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void Box::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Box, Value::Type::kNull, ElementZ::kTop);
}

Box::Box() { set_background_skin(TBIDC("Box"), InvokeInfo::kNoCallbacks); }

}  // namespace elements
}  // namespace el
