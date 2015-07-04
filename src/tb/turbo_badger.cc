/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb_skin.h"
#include "tb_tooltips.h"
#include "tb_widget_animation.h"

#include "tb/animation.h"
#include "tb/config.h"
#include "tb/resources/element_factory.h"
#include "tb/resources/font_manager.h"
#include "tb/resources/font_renderer.h"
#include "tb/resources/image_manager.h"
#include "tb/turbo_badger.h"
#include "tb/util/string_table.h"

namespace tb {

// From elements.cc:
void RegisterBuiltinElementInflaters();

bool Initialize(Renderer* renderer, const char* language_file) {
  assert(!is_initialized());

  Renderer::set(renderer);

  util::StringTable::set(std::make_unique<util::StringTable>());
  util::StringTable::get()->Load(language_file);
  resources::FontManager::set(std::make_unique<resources::FontManager>());
  Skin::set(std::make_unique<Skin>());
  resources::ElementFactory::set(std::make_unique<resources::ElementFactory>());
  resources::ImageManager::set(std::make_unique<resources::ImageManager>());
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
  resources::ImageManager::set(nullptr);
  resources::ElementFactory::set(nullptr);
  Skin::set(nullptr);
  resources::FontManager::set(nullptr);
  util::StringTable::set(nullptr);
  Renderer::set(nullptr);
}

bool is_initialized() { return resources::ElementFactory::get() != nullptr; }

}  // namespace tb
