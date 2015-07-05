/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "elements/tb_widgets_common.h"

#include "tb/animation_manager.h"
#include "tb/element.h"
#include "tb/element_animation.h"
#include "tb/element_animation_manager.h"
#include "tb/elements/message_window.h"
#include "tb/parts/dimmer.h"
#include "tb/window.h"

namespace tb {

using tb::util::SafeCast;

util::TBLinkListOf<ElementAnimation> element_animations;
ElementAnimationManager elements_animation_manager;

void ElementAnimationManager::Init() {
  assert(!element_animations.HasLinks());
  ElementListener::AddGlobalListener(&elements_animation_manager);
}

void ElementAnimationManager::Shutdown() {
  ElementListener::RemoveGlobalListener(&elements_animation_manager);
  assert(!element_animations.HasLinks());
}

void ElementAnimationManager::AbortAnimations(Element* element) {
  AbortAnimations(element, nullptr);
}

void ElementAnimationManager::AbortAnimations(Element* element,
                                              util::tb_type_id_t type_id) {
  auto iter = element_animations.IterateForward();
  while (ElementAnimation* wao = iter.GetAndStep()) {
    if (wao->m_element == element) {
      // Skip this animation if we asked for a specific (and
      // different) animation type.
      if (type_id != nullptr && !wao->IsOfTypeId(type_id)) continue;

      // Abort the animation. This will both autoremove itself
      // and delete it, so no need to do it here.
      AnimationManager::AbortAnimation(wao, true);
    }
  }
}

void ElementAnimationManager::OnElementDelete(Element* element) {
  // Kill and delete all animations running for the element being deleted.
  AbortAnimations(element);
}

bool ElementAnimationManager::OnElementDying(Element* element) {
  bool handled = false;
  if (Window* window = SafeCast<Window>(element)) {
    // Fade out dying windows.
    auto anim = new OpacityElementAnimation(
        window, 1.f, ElementAnimation::kAlmostZeroOpacity, true);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  if (auto window = SafeCast<elements::MessageWindow>(element)) {
    // Move out dying message windows.
    auto anim = new RectElementAnimation(window, Rect(0, 50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaIn);
    AnimationManager::StartAnimation(anim, AnimationCurve::kSpeedUp);
    handled = true;
  }
  if (auto dimmer = SafeCast<parts::Dimmer>(element)) {
    // Fade out dying dim layers.
    auto anim = new OpacityElementAnimation(
        dimmer, 1.f, ElementAnimation::kAlmostZeroOpacity, true);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  return handled;
}

void ElementAnimationManager::OnElementAdded(Element* parent,
                                             Element* element) {
  if (Window* window = SafeCast<Window>(element)) {
    // Fade in new windows.
    auto anim = new OpacityElementAnimation(
        window, ElementAnimation::kAlmostZeroOpacity, 1.f, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
  if (auto window = SafeCast<elements::MessageWindow>(element)) {
    // Move in new message windows.
    auto anim = new RectElementAnimation(window, Rect(0, -50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaOut);
    AnimationManager::StartAnimation(anim);
  }
  if (auto dimmer = SafeCast<parts::Dimmer>(element)) {
    // Fade in dim layer.
    auto anim = new OpacityElementAnimation(
        dimmer, ElementAnimation::kAlmostZeroOpacity, 1.f, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
}

void ElementAnimationManager::OnElementRemove(Element* parent,
                                              Element* element) {}

}  // namespace tb
