/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENT_ANIMATION_H_
#define TB_ELEMENT_ANIMATION_H_

#include "tb/animation.h"
#include "tb/rect.h"

namespace tb {

class Element;

// Base class for element animations. This animation object will be deleted
// automatically if the element is deleted.
class ElementAnimation : public Animation,
                         public util::IntrusiveListEntry<ElementAnimation> {
 public:
  TBOBJECT_SUBCLASS(ElementAnimation, Animation);

  // Don't use 0.0 for opacity animations since that may break focus code.
  // At the moment a window should appear and start fading in from opacity 0,
  // it would also attempt setting the focus to it, but if opacity is 0 it will
  // think focus should not be set in that window and fail.
  static const float kAlmostZeroOpacity;

  ElementAnimation(Element* element);
  ~ElementAnimation() override;

 public:
  Element* m_element;
};

// Animates the opacity of the target element.
class OpacityElementAnimation : public ElementAnimation {
 public:
  TBOBJECT_SUBCLASS(OpacityElementAnimation, ElementAnimation);

  OpacityElementAnimation(Element* element, float src_opacity,
                          float dst_opacity, bool die);

  void OnAnimationStart() override;
  void OnAnimationUpdate(float progress) override;
  void OnAnimationStop(bool aborted) override;

 private:
  float m_src_opacity;
  float m_dst_opacity;
  bool m_die;
};

// Animates the rectangle of the target element.
class RectElementAnimation : public ElementAnimation {
 public:
  TBOBJECT_SUBCLASS(RectElementAnimation, ElementAnimation);

  enum class Mode {
    kSrcToDest,  // Animate from source to dest.
    kDeltaIn,    // Animate from current + delta to current.
    kDeltaOut,   // Animate from current to current + delta.
  };

  // Animates the element between the given source and dest rectangle.
  RectElementAnimation(Element* element, const Rect& src_rect,
                       const Rect& dst_rect);
  // Animates the element between rectangles based on the current element
  // rectangle and a delta. The reference rectangle will be taken from the
  // target element on the first OnAnimationUpdate.
  RectElementAnimation(Element* element, const Rect& delta_rect, Mode mode);

  void OnAnimationStart() override;
  void OnAnimationUpdate(float progress) override;
  void OnAnimationStop(bool aborted) override;

 private:
  Rect m_src_rect;
  Rect m_dst_rect;
  Rect m_delta_rect;
  Mode m_mode;
};

}  // namespace tb

#endif  // TB_ELEMENT_ANIMATION_H_
