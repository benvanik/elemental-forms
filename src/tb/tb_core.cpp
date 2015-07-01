/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_addon.h"
#include "tb_animation.h"
#include "tb_config.h"
#include "tb_core.h"
#include "tb_font_renderer.h"
#include "tb_image_manager.h"
#include "tb_language.h"
#include "tb_skin.h"
#include "tb_system.h"
#include "tb_tooltips.h"
#include "tb_widgets_reader.h"

namespace tb {

TBRenderer* g_renderer = nullptr;
TBSkin* g_tb_skin = nullptr;
TBWidgetsReader* g_widgets_reader = nullptr;
TBLanguage* g_tb_lng = nullptr;
TBFontManager* g_font_manager = nullptr;

bool tb_core_init(TBRenderer* renderer, const char* lng_file) {
  TBDebugPrint("Initiating Turbo Badger - version %s\n", TB_VERSION_STR);
  g_renderer = renderer;
  g_tb_lng = new TBLanguage;
  g_tb_lng->Load(lng_file);
  g_font_manager = new TBFontManager();
  g_tb_skin = new TBSkin();
  g_widgets_reader = TBWidgetsReader::Create();
  g_image_manager = new TBImageManager();

#ifdef TB_SYSTEM_LINUX
  TBSystem::Init();
#endif

  g_tooltip_mng = new TBTooltipManager();

  return TBInitAddons();
}

void tb_core_shutdown() {
  TBAnimationManager::AbortAllAnimations();
  TBShutdownAddons();
  delete g_image_manager;
  delete g_widgets_reader;
  delete g_tb_skin;
  delete g_font_manager;
  delete g_tb_lng;

#ifdef TB_SYSTEM_LINUX
  TBSystem::Shutdown();
#endif
  delete g_tooltip_mng;
}

bool tb_core_is_initialized() { return g_widgets_reader ? true : false; }

}  // namespace tb
