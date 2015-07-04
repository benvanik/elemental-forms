/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_DEBUG_H
#define TB_DEBUG_H

#include "tb/config.h"

#ifdef TB_RUNTIME_DEBUG_INFO
#define TB_IF_DEBUG(debug) debug
#else
#define TB_IF_DEBUG(debug)
#endif  // TB_RUNTIME_DEBUG_INFO

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO

class DebugInfo {
 public:
  DebugInfo();

  enum class Setting {
    // Show elements bounds.
    kLayoutBounds,
    // Show child element clipping set by some elements.
    kLayoutClipping,
    // Show highlights on elements that recalculate their preferred size, and
    // those who recalculate their layout.
    kLayoutSizing,
    // Show render batch info and log batch info in the debug output. It depends
    // on the renderer backend if this is available.
    kDrawRenderBatches,
    // Render the bitmap fragments of the skin.
    kDrawSkinBitmapFragments,
    // Render the bitmap fragments of the font that's set on the hovered or
    // focused element.
    kDrawFontBitmapFragments,

    kSettingCount,
  };
  int settings[int(Setting::kSettingCount)] = {0};
};

extern DebugInfo g_tb_debug;

// Shows a window containing runtime debugging settings.
void ShowDebugInfoSettingsWindow(class Element* root);

#define TB_DEBUG_SETTING(setting) g_tb_debug.settings[int(DebugInfo::setting)]
#define TB_IF_DEBUG_SETTING(setting, code) \
  if (TB_DEBUG_SETTING(setting)) {         \
    code;                                  \
  }

#else  // TB_RUNTIME_DEBUG_INFO

// Shows a window containing runtime debugging settings.
#define ShowDebugInfoSettingsWindow(root) ((void)0)

#define TB_DEBUG_SETTING(setting) false
#define TB_IF_DEBUG_SETTING(setting, code)

#endif  // TB_RUNTIME_DEBUG_INFO

}  // namespace tb

#endif  // TB_DEBUG_H
