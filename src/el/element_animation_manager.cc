/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/animation_manager.h"
#include "el/element.h"
#include "el/element_animation.h"
#include "el/element_animation_manager.h"
#include "el/elements/dimmer.h"
#include "el/elements/form.h"
#include "el/elements/message_form.h"

namespace el {

using el::util::SafeCast;

util::IntrusiveList<ElementAnimation> element_animations;
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
  if (auto form = SafeCast<elements::Form>(element)) {
    // Fade out dying forms.
    auto anim = new OpacityElementAnimation(
        form, 1.f, ElementAnimation::kAlmostZeroOpacity, true);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  if (auto form = SafeCast<elements::MessageForm>(element)) {
    // Move out dying message forms.
    auto anim = new RectElementAnimation(form, Rect(0, 50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaIn);
    AnimationManager::StartAnimation(anim, AnimationCurve::kSpeedUp);
    handled = true;
  }
  if (auto dimmer = SafeCast<elements::Dimmer>(element)) {
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
  if (auto form = SafeCast<elements::Form>(element)) {
    // Fade in new forms.
    auto anim = new OpacityElementAnimation(
        form, ElementAnimation::kAlmostZeroOpacity, 1.f, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
  if (auto form = SafeCast<elements::MessageForm>(element)) {
    // Move in new message forms.
    auto anim = new RectElementAnimation(form, Rect(0, -50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaOut);
    AnimationManager::StartAnimation(anim);
  }
  if (auto dimmer = SafeCast<elements::Dimmer>(element)) {
    // Fade in dim layer.
    auto anim = new OpacityElementAnimation(
        dimmer, ElementAnimation::kAlmostZeroOpacity, 1.f, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
}

void ElementAnimationManager::OnElementRemove(Element* parent,
                                              Element* element) {}

}  // namespace el
