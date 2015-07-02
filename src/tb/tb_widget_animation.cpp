/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widget_animation.h"

#include "tb_list.h"
#include "tb_message_window.h"
#include "tb_window.h"
#include "tb_widgets.h"
#include "tb_widgets_common.h"

namespace tb {

TBLinkListOf<WidgetAnimation> widget_animations;

inline float Lerp(float src, float dst, float progress) {
  return src + (dst - src) * progress;
}

WidgetAnimation::WidgetAnimation(TBWidget* widget) : m_widget(widget) {
  widget_animations.AddLast(this);
}

WidgetAnimation::~WidgetAnimation() { widget_animations.Remove(this); }

OpacityWidgetAnimation::OpacityWidgetAnimation(TBWidget* widget,
                                               float src_opacity,
                                               float dst_opacity, bool die)
    : WidgetAnimation(widget),
      m_src_opacity(src_opacity),
      m_dst_opacity(dst_opacity),
      m_die(die) {}

void OpacityWidgetAnimation::OnAnimationStart() {
  // Make sure we don't stay idle if nothing is scheduled (hack).
  // FIX: fix this properly
  m_widget->Invalidate();

  m_widget->SetOpacity(m_src_opacity);
}

void OpacityWidgetAnimation::OnAnimationUpdate(float progress) {
  m_widget->SetOpacity(Lerp(m_src_opacity, m_dst_opacity, progress));
}

void OpacityWidgetAnimation::OnAnimationStop(bool aborted) {
  // If we're aborted, it may be because the widget is being deleted
  if (m_die && !aborted) {
    TBWidgetSafePointer the_widget(m_widget);
    if (m_widget->GetParent()) m_widget->GetParent()->RemoveChild(m_widget);
    if (the_widget.Get()) delete the_widget.Get();
  } else
    m_widget->SetOpacity(m_dst_opacity);
}

RectWidgetAnimation::RectWidgetAnimation(TBWidget* widget, const Rect& src_rect,
                                         const Rect& dst_rect)
    : WidgetAnimation(widget),
      m_src_rect(src_rect),
      m_dst_rect(dst_rect),
      m_mode(Mode::kSrcToDest) {}

RectWidgetAnimation::RectWidgetAnimation(TBWidget* widget,
                                         const Rect& delta_rect, Mode mode)
    : WidgetAnimation(widget), m_delta_rect(delta_rect), m_mode(mode) {
  assert(mode == Mode::kDeltaIn || mode == Mode::kDeltaOut);
}

void RectWidgetAnimation::OnAnimationStart() {
  // Make sure we don't stay idle if nothing is scheduled (hack).
  // FIX: fix this properly
  m_widget->Invalidate();

  if (m_mode == Mode::kSrcToDest) {
    m_widget->SetRect(m_src_rect);
  }
}

void RectWidgetAnimation::OnAnimationUpdate(float progress) {
  if (m_mode == Mode::kDeltaIn || m_mode == Mode::kDeltaOut) {
    m_dst_rect = m_src_rect = m_widget->GetRect();
    if (m_dst_rect.Equals(Rect())) {
      // Widget hasn't been laid out yet,
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
  m_widget->SetRect(rect);
}

void RectWidgetAnimation::OnAnimationStop(bool aborted) {
  if (m_mode == Mode::kSrcToDest) {
    // m_dst_rect may still be unset if aborted.
    m_widget->SetRect(m_dst_rect);
  }
}

WidgetAnimationManager widgets_animation_manager;

void WidgetAnimationManager::Init() {
  TBWidgetListener::AddGlobalListener(&widgets_animation_manager);
}

void WidgetAnimationManager::Shutdown() {
  TBWidgetListener::RemoveGlobalListener(&widgets_animation_manager);
}

void WidgetAnimationManager::AbortAnimations(TBWidget* widget) {
  AbortAnimations(widget, nullptr);
}

void WidgetAnimationManager::AbortAnimations(TBWidget* widget,
                                             TB_TYPE_ID type_id) {
  auto iter = widget_animations.IterateForward();
  while (WidgetAnimation* wao = iter.GetAndStep()) {
    if (wao->m_widget == widget) {
      // Skip this animation if we asked for a specific (and
      // different) animation type.
      if (type_id != nullptr && !wao->IsOfTypeId(type_id)) continue;

      // Abort the animation. This will both autoremove itself
      // and delete it, so no need to do it here.
      AnimationManager::AbortAnimation(wao, true);
    }
  }
}

void WidgetAnimationManager::OnWidgetDelete(TBWidget* widget) {
  // Kill and delete all animations running for the widget being deleted.
  AbortAnimations(widget);
}

bool WidgetAnimationManager::OnWidgetDying(TBWidget* widget) {
  bool handled = false;
  if (Window* window = TBSafeCast<Window>(widget)) {
    // Fade out dying windows.
    if (auto anim =
            new OpacityWidgetAnimation(window, 1.f, kAlmostZeroOpacity, true))
      AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  if (MessageWindow* window = TBSafeCast<MessageWindow>(widget)) {
    // Move out dying message windows.
    if (auto anim = new RectWidgetAnimation(
            window, Rect(0, 50, 0, 0), RectWidgetAnimation::Mode::kDeltaIn))
      AnimationManager::StartAnimation(anim, AnimationCurve::kSpeedUp);
    handled = true;
  }
  if (TBDimmer* dimmer = TBSafeCast<TBDimmer>(widget)) {
    // Fade out dying dim layers.
    if (auto anim =
            new OpacityWidgetAnimation(dimmer, 1.f, kAlmostZeroOpacity, true))
      AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
    handled = true;
  }
  return handled;
}

void WidgetAnimationManager::OnWidgetAdded(TBWidget* parent, TBWidget* widget) {
  if (Window* window = TBSafeCast<Window>(widget)) {
    // Fade in new windows
    if (auto anim =
            new OpacityWidgetAnimation(window, kAlmostZeroOpacity, 1.f, false))
      AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
  if (MessageWindow* window = TBSafeCast<MessageWindow>(widget)) {
    // Move in new message windows
    if (auto anim = new RectWidgetAnimation(
            window, Rect(0, -50, 0, 0), RectWidgetAnimation::Mode::kDeltaOut))
      AnimationManager::StartAnimation(anim);
  }
  if (TBDimmer* dimmer = TBSafeCast<TBDimmer>(widget)) {
    // Fade in dim layer
    if (auto anim =
            new OpacityWidgetAnimation(dimmer, kAlmostZeroOpacity, 1.f, false))
      AnimationManager::StartAnimation(anim, AnimationCurve::kBezier);
  }
}

void WidgetAnimationManager::OnWidgetRemove(TBWidget* parent,
                                            TBWidget* widget) {}

}  // namespace tb
