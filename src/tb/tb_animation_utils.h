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

// An animated float value.
class FloatAnimation : public Animation {
 public:
  float src_val;
  float dst_val;
  float current_progress;

 public:
  TBOBJECT_SUBCLASS(FloatAnimation, Animation);

  FloatAnimation(float initial_value,
                 AnimationCurve animation_curve = Animation::kDefaultCurve,
                 double animation_duration = Animation::kDefaultDuration)
      : src_val(initial_value), dst_val(initial_value), current_progress(0) {
    Animation::animation_curve = animation_curve;
    Animation::animation_duration = animation_duration;
  }

  float GetValue() { return src_val + (dst_val - src_val) * current_progress; }
  void SetValueAnimated(float value) {
    src_val = GetValue();
    dst_val = value;
    AnimationManager::StartAnimation(this, animation_curve, animation_duration);
  }
  void SetValueImmediately(float value) {
    AnimationManager::AbortAnimation(this, false);
    src_val = dst_val = value;
    OnAnimationUpdate(1.0f);
  }

  void OnAnimationStart() override { current_progress = 0; }
  void OnAnimationUpdate(float progress) override {
    current_progress = progress;
  }
  void OnAnimationStop(bool aborted) override {}
};

// Animates a external float value, which address is given in the constructor.
class ExternalFloatAnimation : public FloatAnimation {
 public:
  float* target_value;

 public:
  TBOBJECT_SUBCLASS(ExternalFloatAnimation, Animation);

  ExternalFloatAnimation(
      float* target_value,
      AnimationCurve animation_curve = Animation::kDefaultCurve,
      double animation_duration = Animation::kDefaultDuration)
      : FloatAnimation(*target_value), target_value(target_value) {}

  void OnAnimationStart() override {
    FloatAnimation::OnAnimationStart();
    *target_value = GetValue();
  }
  void OnAnimationUpdate(float progress) override {
    FloatAnimation::OnAnimationUpdate(progress);
    *target_value = GetValue();
  }
};

}  // namespace tb

#endif  // TB_ANIMATION_UTILS_H
