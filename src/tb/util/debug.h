/**
******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_DEBUG_H_
#define TB_UTIL_DEBUG_H_

#include <string>

#include "tb/config.h"

#ifdef TB_RUNTIME_DEBUG_INFO
void TBDebugOut(const char* str, ...);
inline void TBDebugOut(const std::string& str) { TBDebugOut(str.c_str()); }
#define TB_IF_DEBUG(debug) debug
#else
#define TBDebugOut(str, ...) ((void)0)
#define TB_IF_DEBUG(debug)
#endif  // TB_RUNTIME_DEBUG_INFO

#ifdef TB_RUNTIME_DEBUG_INFO

namespace tb {
class Element;
}  // namespace tb

namespace tb {
namespace util {

class DebugInfo {
 public:
  static DebugInfo* get() { return &debug_info_singleton_; }

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

 private:
  static DebugInfo debug_info_singleton_;
};

// Shows a window containing runtime debugging settings.
void ShowDebugInfoSettingsWindow(Element* root);

#define TB_DEBUG_SETTING(setting) \
  tb::util::DebugInfo::get()->settings[int(setting)]
#define TB_IF_DEBUG_SETTING(setting, code) \
  if (TB_DEBUG_SETTING(setting)) {         \
    code;                                  \
  }

}  // namespace util
}  // namespace tb

#else
#define ShowDebugInfoSettingsWindow(root) ((void)0)
#define TB_DEBUG_SETTING(setting) false
#define TB_IF_DEBUG_SETTING(setting, code)
#endif  // TB_RUNTIME_DEBUG_INFO

#endif  // TB_UTIL_DEBUG_H_
