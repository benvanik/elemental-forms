/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/graphics/renderer.h"

namespace tb {
namespace graphics {

Renderer* Renderer::renderer_singleton_ = nullptr;

Renderer::~Renderer() {
  if (renderer_singleton_ == this) {
    renderer_singleton_ = nullptr;
  }
}

void Renderer::InvokeContextLost() {
  auto iter = listeners_.IterateForward();
  while (RendererListener* listener = iter.GetAndStep()) {
    listener->OnContextLost();
  }
}

void Renderer::InvokeContextRestored() {
  auto iter = listeners_.IterateForward();
  while (RendererListener* listener = iter.GetAndStep()) {
    listener->OnContextRestored();
  }
}

}  // namespace graphics
}  // namespace tb
