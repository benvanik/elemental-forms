/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widget_animation.h"

#include "tb_message_window.h"
#include "tb_window.h"
#include "tb_widgets.h"
#include "tb_widgets_common.h"

namespace tb {

util::TBLinkListOf<ElementAnimation> element_animations;

inline float Lerp(float src, float dst, float progress) {
  return src + (dst - src) * progress;
}

ElementAnimation::ElementAnimation(Element* element) : m_element(element) {
  element_animations.AddLast(this);
}

ElementAnimation::~ElementAnimation() { element_animations.Remove(this); }

OpacityElementAnimation::OpacityElementAnimation(Element* element,
                                                 float src_opacity,
                                                 float dst_opacity, bool die)
    : ElementAnimation(element),
      m_src_opacity(src_opacity),
      m_dst_opacity(dst_opacity),
      m_die(die) {}

void OpacityElementAnimation::OnAnimationStart() {
  // Make sure we don't stay idle if nothing is scheduled (hack).
  // FIX: fix this properly
  m_element->Invalidate();

  m_element->SetOpacity(m_src_opacity);
}

void OpacityElementAnimation::OnAnimationUpdate(float progress) {
  m_element->SetOpacity(Lerp(m_src_opacity, m_dst_opacity, progress));
}

void OpacityElementAnimation::OnAnimationStop(bool aborted) {
  // If we're aborted, it may be because the element is being deleted
  if (m_die && !aborted) {
    WeakElementPointer the_element(m_element);
    if (m_element->GetParent()) m_element->GetParent()->RemoveChild(m_element);
    if (the_element.Get()) delete the_element.Get();
  } else
    m_element->SetOpacity(m_dst_opacity);
}

RectElementAnimation::RectElementAnimation(Element* element,
                                           const Rect& src_rect,
                                           const Rect& dst_rect)
    : ElementAnimation(element),
      m_src_rect(src_rect),
      m_dst_rect(dst_rect),
      m_mode(Mode::kSrcToDest) {}

RectElementAnimation::RectElementAnimation(Element* element,
                                           const Rect& delta_rect, Mode mode)
    : ElementAnimation(element), m_delta_rect(delta_rect), m_mode(mode) {
  assert(mode == Mode::kDeltaIn || mode == Mode::kDeltaOut);
}

void RectElementAnimation::OnAnimationStart() {
  // Make sure we don't stay idle if nothing is scheduled (hack).
  // FIX: fix this properly
  m_element->Invalidate();

  if (m_mode == Mode::kSrcToDest) {
    m_element->set_rect(m_src_rect);
  }
}

void RectElementAnimation::OnAnimationUpdate(float progress) {
  if (m_mode == Mode::kDeltaIn || m_mode == Mode::kDeltaOut) {
    m_dst_rect = m_src_rect = m_element->rect();
    if (m_dst_rect.equals(Rect())) {
      // Element hasn't been laid out yet,
      // the animation was started too soon.
      AnimationManager::AbortAnimation(this, true);
      return;
    }
    if (m_mode == Mode::kDeltaIn) {
      m_dst_rect.x += m_delta_rect.x;
      m_dst_rect.y += m_delta_rect.y;
      m_dst_rect.w += m_delta_rect.w;
      m_dst_rect.h += m_delta_rect.h;
    } else {
      m_src_rect.x += m_delta_rect.x;
      m_src_rect.y += m_delta_rect.y;
      m_src_rect.w += m_delta_rect.w;
      m_src_rect.h += m_delta_rect.h;
    }
    m_mode = Mode::kSrcToDest;
  }
  Rect rect;
  rect.x = int(Lerp(float(m_src_rect.x), float(m_dst_rect.x), progress));
  rect.y = int(Lerp(float(m_src_rect.y), float(m_dst_rect.y), progress));
  rect.w = int(Lerp(float(m_src_rect.w), float(m_dst_rect.w), progress));
  rect.h = int(Lerp(float(m_src_rect.h), float(m_dst_rect.h), progress));
  m_element->set_rect(rect);
}

void RectElementAnimation::OnAnimationStop(bool aborted) {
  if (m_mode == Mode::kSrcToDest) {
    // m_dst_rect may still be unset if aborted.
    m_element->set_rect(m_dst_rect);
  }
}

ElementAnimationManager elements_animation_manager;

void ElementAnimationManager::Init() {
  ElementListener::AddGlobalListener(&elements_animation_manager);
}

void ElementAnimationManager::Shutdown() {
  ElementListener::RemoveGlobalListener(&elements_animation_manager);
}

void ElementAnimationManager::AbortAnimations(Element* element) {
  AbortAnimations(element, nullptr);
}

void ElementAnimationManager::AbortAnimations(Element* element,
                                              tb_type_id_t type_id) {
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
  if (Window* window = TBSafeCast<Window>(element)) {
    // Fade out dying windows.
    auto anim =
        new OpacityElementAnimation(window, 1.f, kAlmostZeroOpacity, true);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  if (MessageWindow* window = TBSafeCast<MessageWindow>(element)) {
    // Move out dying message windows.
    auto anim = new RectElementAnimation(window, Rect(0, 50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaIn);
    AnimationManager::StartAnimation(anim, AnimationCurve::kSpeedUp);
    handled = true;
  }
  if (Dimmer* dimmer = TBSafeCast<Dimmer>(element)) {
    // Fade out dying dim layers.
    auto anim =
        new OpacityElementAnimation(dimmer, 1.f, kAlmostZeroOpacity, true);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  return handled;
}

void ElementAnimationManager::OnElementAdded(Element* parent,
                                             Element* element) {
  if (Window* window = TBSafeCast<Window>(element)) {
    // Fade in new windows.
    auto anim =
        new OpacityElementAnimation(window, kAlmostZeroOpacity, 1.f, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
  if (MessageWindow* window = TBSafeCast<MessageWindow>(element)) {
    // Move in new message windows.
    auto anim = new RectElementAnimation(window, Rect(0, -50, 0, 0),
                                         RectElementAnimation::Mode::kDeltaOut);
    AnimationManager::StartAnimation(anim);
  }
  if (Dimmer* dimmer = TBSafeCast<Dimmer>(element)) {
    // Fade in dim layer.
    auto anim =
        new OpacityElementAnimation(dimmer, kAlmostZeroOpacity, 1.f, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
}

void ElementAnimationManager::OnElementRemove(Element* parent,
                                              Element* element) {}

}  // namespace tb
