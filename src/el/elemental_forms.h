/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTAL_FORMS_H_
#define EL_ELEMENTAL_FORMS_H_

#include "el/config.h"
#include "el/elements.h"
#include "el/util/string.h"

namespace el {
namespace graphics {
class Renderer;
}  // namespace graphics
}  // namespace el

namespace el {

using el::util::format_string;

// Initializes Elemental Forms. Call this before using any elemental API.
bool Initialize(graphics::Renderer* renderer, const char* language_file);

// Shuts down Elemental Forms. Call this after deleting the last element, to
// free libelemental internals.
void Shutdown();

// Returns true if the Elemental Forms library is initialized.
bool is_initialized();

}  // namespace el

#endif  // EL_ELEMENTAL_FORMS_H_
