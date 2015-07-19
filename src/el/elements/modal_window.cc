/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cassert>

#include "el/animation_manager.h"
#include "el/element_animation.h"
#include "el/elements/dimmer.h"
#include "el/elements/modal_window.h"

namespace el {
namespace elements {

int ModalWindow::visible_count_ = 0;

ModalWindow::ModalWindow(std::function<void()> on_close_callback)
    : on_close_callback_(on_close_callback) {
  ++visible_count_;
  set_settings(el::WindowSettings::kTitleBar |
               el::WindowSettings::kCloseButton |
               el::WindowSettings::kCanActivate);
  el::ElementListener::AddGlobalListener(this);
}

ModalWindow::~ModalWindow() {
  --visible_count_;
  el::ElementListener::RemoveGlobalListener(this);
  if (auto dimmer = dimmer_.get()) {
    dimmer->RemoveFromParent();
    delete dimmer;
  }
}

void ModalWindow::Show(el::Element* root_element) {
  auto dimmer = new Dimmer();
  root_element->AddChild(dimmer);
  dimmer_.reset(dimmer);

  BuildUI();

  root_element->AddChild(this);
  ResizeToFitContent();
  CenterInParent();
}

bool ModalWindow::OnEvent(const el::Event& ev) {
  if (ev.target->id() == TBIDC("exit_button") &&
      ev.type == el::EventType::kClick) {
    Die();
    return true;
  } else if (ev.type == el::EventType::kKeyDown &&
             ev.special_key == el::SpecialKey::kEsc) {
    Die();
    return true;
  }
  return el::Window::OnEvent(ev);
}

void ModalWindow::OnDie() {
  if (auto dimmer = dimmer_.get()) {
    dimmer->Die();
  }
  Window::OnDie();
}

void ModalWindow::OnRemove() { on_close_callback_(); }

void ModalWindow::OnElementAdded(Element* parent, Element* element) {
  if (element == this) {
    // Move in.
    auto anim = new RectElementAnimation(this, Rect(0, -50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaOut);
    AnimationManager::StartAnimation(anim);
  }
}

bool ModalWindow::OnElementDying(Element* element) {
  if (element == this) {
    // Move out.
    auto anim = new RectElementAnimation(this, Rect(0, 50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaIn);
    AnimationManager::StartAnimation(anim, AnimationCurve::kSpeedUp);
    return true;
  }
  return false;
}

}  // namespace elements
}  // namespace el
