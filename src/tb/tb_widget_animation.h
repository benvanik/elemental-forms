/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_WIDGET_ANIMATION_H
#define TB_WIDGET_ANIMATION_H

#include "tb/animation.h"
#include "tb_widgets_listener.h"

namespace tb {

// Don't use 0.0 for opacity animations since that may break focus code.
// At the moment a window should appear and start fading in from opacity 0,
// it would also attempt setting the focus to it, but if opacity is 0 it will
// think focus should not be set in that window and fail.
constexpr float kAlmostZeroOpacity = 0.001f;

// Base class for element animations. This animation object will be deleted
// automatically if the element is deleted.
class ElementAnimation : public Animation,
                         public util::TBLinkOf<ElementAnimation> {
 public:
  TBOBJECT_SUBCLASS(ElementAnimation, Animation);

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

class ElementAnimationManager : public ElementListener {
 public:
  static void Init();
  static void Shutdown();

  // Aborts all animations that are running for the given element.
  static void AbortAnimations(Element* element);

  // Abort all animations matching the given type that are running for the given
  // element.
  // This example will abort all opacity animations:
  // AbortAnimations(element,
  //     TypedObject::GetTypeId<OpacityElementAnimation>())
  static void AbortAnimations(Element* element, util::tb_type_id_t type_id);

 private:
  void OnElementDelete(Element* element) override;
  bool OnElementDying(Element* element) override;
  void OnElementAdded(Element* parent, Element* child) override;
  void OnElementRemove(Element* parent, Element* child) override;
};

}  // namespace tb

#endif  // TB_WIDGET_ANIMATION_H
