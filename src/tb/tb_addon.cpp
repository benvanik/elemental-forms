/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_addon.h"

#include "tb_system.h"

namespace tb {

// We can't use a linked list object since we don't know if its constructor
// would run before of after any widget factory constructor that add itself
// to it. Using a manual one way link list is very simple.
TBAddonFactory* g_registered_addon_factories = nullptr;

TBAddonFactory::TBAddonFactory() : next(g_registered_addon_factories) {
  g_registered_addon_factories = this;
}

TBLinkListOf<TBAddon> m_addons;

bool TBInitAddons() {
  TBAddonFactory* f = g_registered_addon_factories;
  while (f) {
    TBAddon* addon = f->Create();
    if (!addon || !addon->Init()) {
      delete addon;
      TBDebugOut("Failed initiating addon\n");
      return false;
    }
    m_addons.AddLast(addon);
    f = f->next;
  }
  return true;
}

void TBShutdownAddons() {
  while (TBAddon* addon = m_addons.GetLast()) {
    addon->Shutdown();
    m_addons.Delete(addon);
  }
}

}  // namespace tb
