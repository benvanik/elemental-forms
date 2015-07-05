/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_widgets_common.h"

#include "tb/animation_manager.h"
#include "tb/element.h"
#include "tb/element_animation.h"
#include "tb/element_listener.h"

namespace tb {

using tb::util::SafeCast;

extern util::TBLinkListOf<ElementAnimation> element_animations;

inline float Lerp(float src, float dst, float progress) {
  return src + (dst - src) * progress;
}

const float ElementAnimation::kAlmostZeroOpacity = 0.001f;

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

}  // namespace tb
