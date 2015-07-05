/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb/animation_manager.h"
#include "tb/config.h"
#include "tb/element_animation_manager.h"
#include "tb/graphics/image_manager.h"
#include "tb/parsing/element_factory.h"
#include "tb/resources/font_manager.h"
#include "tb/resources/font_renderer.h"
#include "tb/resources/skin.h"
#include "tb/tooltip_manager.h"
#include "tb/turbo_badger.h"
#include "tb/util/string_table.h"

namespace tb {

using graphics::Renderer;

// From elements.cc:
void RegisterBuiltinElementInflaters();

bool Initialize(Renderer* renderer, const char* language_file) {
  assert(!is_initialized());

  Renderer::set(renderer);

  util::StringTable::set(std::make_unique<util::StringTable>());
  util::StringTable::get()->Load(language_file);
  resources::FontManager::set(std::make_unique<resources::FontManager>());
  resources::Skin::set(std::make_unique<resources::Skin>());
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
  resources::Skin::set(nullptr);
  resources::FontManager::set(nullptr);
  util::StringTable::set(nullptr);
  Renderer::set(nullptr);
}

bool is_initialized() { return parsing::ElementFactory::get() != nullptr; }

}  // namespace tb
