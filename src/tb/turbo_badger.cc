/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb_font_renderer.h"
#include "tb_image_manager.h"
#include "tb_language.h"
#include "tb_skin.h"
#include "tb_tooltips.h"
#include "tb_widget_animation.h"
#include "tb_widgets_reader.h"

#include "tb/animation.h"
#include "tb/config.h"
#include "tb/turbo_badger.h"

namespace tb {

bool Initialize(Renderer* renderer, const char* language_file) {
  assert(!is_initialized());

  Renderer::set(renderer);

  Language::set(std::make_unique<Language>());
  Language::get()->Load(language_file);
  FontManager::set(std::make_unique<FontManager>());
  Skin::set(std::make_unique<Skin>());
  ElementReader::set(std::make_unique<ElementReader>());
  ImageManager::set(std::make_unique<ImageManager>());
  TooltipManager::set(std::make_unique<TooltipManager>());

  ElementAnimationManager::Init();

  return true;
}

void Shutdown() {
  if (!is_initialized()) {
    return;
  }

  AnimationManager::AbortAllAnimations();
  ElementAnimationManager::Shutdown();

  TooltipManager::set(nullptr);
  ImageManager::set(nullptr);
  ElementReader::set(nullptr);
  Skin::set(nullptr);
  FontManager::set(nullptr);
  Language::set(nullptr);
  Renderer::set(nullptr);
}

bool is_initialized() { return ElementReader::get() ? true : false; }

}  // namespace tb
