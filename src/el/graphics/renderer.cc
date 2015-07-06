/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/graphics/renderer.h"

namespace el {
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
}  // namespace el
