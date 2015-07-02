/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_ANIMATION_UTILS_H
#define TB_ANIMATION_UTILS_H

#include "tb_animation.h"

namespace tb {

// TBAnimatedFloat - A animated float value
class TBAnimatedFloat : public TBAnimationObject {
 public:
  float src_val;
  float dst_val;
  float current_progress;

 public:
  // For safe typecasting
  TBOBJECT_SUBCLASS(TBAnimatedFloat, TBAnimationObject);

  TBAnimatedFloat(float initial_value,
                  AnimationCurve animation_curve = ANIMATION_DEFAULT_CURVE,
                  double animation_duration = ANIMATION_DEFAULT_DURATION)
      : src_val(initial_value), dst_val(initial_value), current_progress(0) {
    TBAnimationObject::animation_curve = animation_curve;
    TBAnimationObject::animation_duration = animation_duration;
  }

  float GetValue() { return src_val + (dst_val - src_val) * current_progress; }
  void SetValueAnimated(float value) {
    src_val = GetValue();
    dst_val = value;
    TBAnimationManager::StartAnimation(this, animation_curve,
                                       animation_duration);
  }
  void SetValueImmediately(float value) {
    TBAnimationManager::AbortAnimation(this, false);
    src_val = dst_val = value;
    OnAnimationUpdate(1.0f);
  }

  virtual void OnAnimationStart() { current_progress = 0; }
  virtual void OnAnimationUpdate(float progress) {
    current_progress = progress;
  }
  virtual void OnAnimationStop(bool aborted) {}
};

// TBFloatAnimator - Animates a external float value, which address is given in
// the constructor.
class TBFloatAnimator : public TBAnimatedFloat {
 public:
  float* target_value;

 public:
  // For safe typecasting
  TBOBJECT_SUBCLASS(TBFloatAnimator, TBAnimationObject);

  TBFloatAnimator(float* target_value,
                  AnimationCurve animation_curve = ANIMATION_DEFAULT_CURVE,
                  double animation_duration = ANIMATION_DEFAULT_DURATION)
      : TBAnimatedFloat(*target_value), target_value(target_value) {}

  virtual void OnAnimationStart() {
    TBAnimatedFloat::OnAnimationStart();
    *target_value = GetValue();
  }
  virtual void OnAnimationUpdate(float progress) {
    TBAnimatedFloat::OnAnimationUpdate(progress);
    *target_value = GetValue();
  }
};

}  // namespace tb

#endif  // TB_ANIMATION_UTILS_H
