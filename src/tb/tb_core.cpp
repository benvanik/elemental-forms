/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_core.h"

#include "tb_animation.h"
#include "tb/config.h"
#include "tb_font_renderer.h"
#include "tb_image_manager.h"
#include "tb_language.h"
#include "tb_skin.h"
#include "tb_system.h"
#include "tb_tooltips.h"
#include "tb_widgets_reader.h"

namespace tb {

FontManager* g_font_manager = nullptr;
Language* g_tb_lng = nullptr;
Renderer* g_renderer = nullptr;
Skin* g_tb_skin = nullptr;
ElementReader* g_elements_reader = nullptr;

bool tb_core_init(Renderer* renderer, const char* lng_file) {
  g_renderer = renderer;
  g_tb_lng = new Language;
  g_tb_lng->Load(lng_file);
  g_font_manager = new FontManager();
  g_tb_skin = new Skin();
  g_elements_reader = ElementReader::Create();
  g_image_manager = new ImageManager();

#ifdef TB_SYSTEM_LINUX
  TBSystem::Init();
#endif  // TB_SYSTEM_LINUX

  g_tooltip_mng = new TooltipManager();

  return true;
}

void tb_core_shutdown() {
  AnimationManager::AbortAllAnimations();
  delete g_image_manager;
  delete g_elements_reader;
  delete g_tb_skin;
  delete g_font_manager;
  delete g_tb_lng;

#ifdef TB_SYSTEM_LINUX
  TBSystem::Shutdown();
#endif  // TB_SYSTEM_LINUX

  delete g_tooltip_mng;
}

bool tb_core_is_initialized() { return g_elements_reader ? true : false; }

}  // namespace tb
