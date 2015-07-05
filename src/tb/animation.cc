/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>

#include "tb/animation.h"
#include "tb/animation_manager.h"

namespace tb {

void Animation::InvokeOnAnimationStart() {
  auto li = m_listeners.IterateForward();
  OnAnimationStart();
  while (AnimationListener* listener = li.GetAndStep()) {
    listener->OnAnimationStart(this);
  }
}

void Animation::InvokeOnAnimationUpdate(float progress) {
  auto li = m_listeners.IterateForward();
  OnAnimationUpdate(progress);
  while (AnimationListener* listener = li.GetAndStep()) {
    listener->OnAnimationUpdate(this, progress);
  }
}

void Animation::InvokeOnAnimationStop(bool aborted) {
  auto li = m_listeners.IterateForward();
  OnAnimationStop(aborted);
  while (AnimationListener* listener = li.GetAndStep()) {
    listener->OnAnimationStop(this, aborted);
  }
}

void FloatAnimation::set_value_animated(float new_value) {
  src_val = value();
  dst_val = new_value;
  AnimationManager::StartAnimation(this, animation_curve, animation_duration);
}

void FloatAnimation::set_value_immediately(float new_value) {
  AnimationManager::AbortAnimation(this, false);
  src_val = dst_val = new_value;
  OnAnimationUpdate(1.0f);
}

}  // namespace tb
