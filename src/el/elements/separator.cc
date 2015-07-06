/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/separator.h"
#include "el/parsing/element_inflater.h"

namespace el {
namespace elements {

void Separator::RegisterInflater() {
  EL_REGISTER_ELEMENT_INFLATER(Separator, Value::Type::kNull, ElementZ::kTop);
}

Separator::Separator() {
  set_background_skin(TBIDC("Separator"), InvokeInfo::kNoCallbacks);
  set_state(Element::State::kDisabled, true);
}

}  // namespace elements
}  // namespace el
