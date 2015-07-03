/** @mainpage Turbo Badger - Fast UI toolkit

Turbo Badger
Copyright (C) 2011-2014 Emil Segerås

License:

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef TB_CORE_H
#define TB_CORE_H

#include "tb_debug.h"
#include "tb_hash.h"
#include "tb_types.h"

namespace tb {

// TODO(benvanik): remove all of these.

class FontManager;
extern FontManager* g_font_manager;

class Language;
extern Language* g_tb_lng;

class Renderer;
extern Renderer* g_renderer;

class Skin;
extern Skin* g_tb_skin;

class ElementReader;
extern ElementReader* g_elements_reader;

// Initializes turbo badger. Call this before using any turbo badger API.
bool tb_core_init(Renderer* renderer, const char* lng_file);

// Shuts down turbo badger. Call this after deleting the last element, to free
// turbo badger internals.
void tb_core_shutdown();

// Returns true if turbo badger is initialized.
bool tb_core_is_initialized();

}  // namespace tb

#endif  // TB_CORE_H
