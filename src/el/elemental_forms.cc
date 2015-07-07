/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cassert>

#include "el/animation_manager.h"
#include "el/config.h"
#include "el/element_animation_manager.h"
#include "el/elemental_forms.h"
#include "el/graphics/image_manager.h"
#include "el/parsing/element_factory.h"
#include "el/text/font_manager.h"
#include "el/text/font_renderer.h"
#include "el/skin.h"
#include "el/tooltip_manager.h"
#include "el/util/string_table.h"

namespace el {

using graphics::Renderer;

// From elements.cc:
void RegisterBuiltinElementInflaters();

bool Initialize(Renderer* renderer) {
  assert(!is_initialized());

  Renderer::set(renderer);

  util::StringTable::set(std::make_unique<util::StringTable>());
  text::FontManager::set(std::make_unique<text::FontManager>());
  Skin::set(std::make_unique<Skin>());
  parsing::ElementFactory::set(std::make_unique<parsing::ElementFactory>());
  graphics::ImageManager::set(std::make_unique<graphics::ImageManager>());
  TooltipManager::set(std::make_unique<TooltipManager>());

  ElementAnimationManager::Init();

  // Once at init for all our built in elements.
  // Any custom elements will have to be registered by the owner.
  RegisterBuiltinElementInflaters();

  return true;
}

void Shutdown() {
  if (!is_initialized()) {
    return;
  }

  AnimationManager::AbortAllAnimations();
  ElementAnimationManager::Shutdown();

  TooltipManager::set(nullptr);
  graphics::ImageManager::set(nullptr);
  parsing::ElementFactory::set(nullptr);
  Skin::set(nullptr);
  text::FontManager::set(nullptr);
  util::StringTable::set(nullptr);
  Renderer::set(nullptr);
}

bool is_initialized() { return parsing::ElementFactory::get() != nullptr; }

}  // namespace el
