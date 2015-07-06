/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/animation_manager.h"
#include "el/element.h"
#include "el/element_animation.h"
#include "el/element_listener.h"

namespace el {

using el::util::SafeCast;

extern util::IntrusiveList<ElementAnimation> element_animations;

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

  m_element->set_opacity(m_src_opacity);
}

void OpacityElementAnimation::OnAnimationUpdate(float progress) {
  m_element->set_opacity(Lerp(m_src_opacity, m_dst_opacity, progress));
}

void OpacityElementAnimation::OnAnimationStop(bool aborted) {
  // If we're aborted, it may be because the element is being deleted
  if (m_die && !aborted) {
    WeakElementPointer the_element(m_element);
    if (m_element->parent()) m_element->parent()->RemoveChild(m_element);
    if (the_element.get()) delete the_element.get();
  } else
    m_element->set_opacity(m_dst_opacity);
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

}  // namespace el
