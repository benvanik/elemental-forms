/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_renderer.h"

namespace tb {

void Renderer::InvokeContextLost() {
  auto iter = m_listeners.IterateForward();
  while (RendererListener* listener = iter.GetAndStep()) {
    listener->OnContextLost();
  }
}

void Renderer::InvokeContextRestored() {
  auto iter = m_listeners.IterateForward();
  while (RendererListener* listener = iter.GetAndStep()) {
    listener->OnContextRestored();
  }
}

}  // namespace tb
