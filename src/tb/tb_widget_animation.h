/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WIDGET_ANIMATION_H
#define TB_WIDGET_ANIMATION_H

#include "tb_animation.h"
#include "tb_widgets_listener.h"

namespace tb {

// Don't use 0.0 for opacity animations since that may break focus code.
// At the moment a window should appear and start fading in from opacity 0,
// it would also attempt setting the focus to it, but if opacity is 0 it will
// think focus should not be set in that window and fail.
constexpr float kAlmostZeroOpacity = 0.001f;

// Base class for widget animations. This animation object will be deleted
// automatically if the widget is deleted.
class WidgetAnimation : public Animation, public TBLinkOf<WidgetAnimation> {
 public:
  TBOBJECT_SUBCLASS(WidgetAnimation, Animation);

  WidgetAnimation(TBWidget* widget);
  ~WidgetAnimation() override;

 public:
  TBWidget* m_widget;
};

// Animates the opacity of the target widget.
class OpacityWidgetAnimation : public WidgetAnimation {
 public:
  TBOBJECT_SUBCLASS(OpacityWidgetAnimation, WidgetAnimation);

  OpacityWidgetAnimation(TBWidget* widget, float src_opacity, float dst_opacity,
                         bool die);

  void OnAnimationStart() override;
  void OnAnimationUpdate(float progress) override;
  void OnAnimationStop(bool aborted) override;

 private:
  float m_src_opacity;
  float m_dst_opacity;
  bool m_die;
};

// Animates the rectangle of the target widget.
class RectWidgetAnimation : public WidgetAnimation {
 public:
  TBOBJECT_SUBCLASS(RectWidgetAnimation, WidgetAnimation);

  enum class Mode {
    kSrcToDest,  // Animate from source to dest.
    kDeltaIn,    // Animate from current + delta to current.
    kDeltaOut,   // Animate from current to current + delta.
  };

  // Animates the widget between the given source and dest rectangle.
  RectWidgetAnimation(TBWidget* widget, const Rect& src_rect,
                      const Rect& dst_rect);
  // Animates the widget between rectangles based on the current widget
  // rectangle and a delta. The reference rectangle will be taken from the
  // target widget on the first OnAnimationUpdate.
  RectWidgetAnimation(TBWidget* widget, const Rect& delta_rect, Mode mode);

  void OnAnimationStart() override;
  void OnAnimationUpdate(float progress) override;
  void OnAnimationStop(bool aborted) override;

 private:
  Rect m_src_rect;
  Rect m_dst_rect;
  Rect m_delta_rect;
  Mode m_mode;
};

class WidgetAnimationManager : public WidgetListener {
 public:
  static void Init();
  static void Shutdown();

  // Aborts all animations that are running for the given widget.
  static void AbortAnimations(TBWidget* widget);

  // Abort all animations matching the given type that are running for the given
  // widget.
  // This example will abort all opacity animations:
  // AbortAnimations(widget,
  //     TBTypedObject::GetTypeId<OpacityWidgetAnimation>())
  static void AbortAnimations(TBWidget* widget, TB_TYPE_ID type_id);

 private:
  void OnWidgetDelete(TBWidget* widget) override;
  bool OnWidgetDying(TBWidget* widget) override;
  void OnWidgetAdded(TBWidget* parent, TBWidget* child) override;
  void OnWidgetRemove(TBWidget* parent, TBWidget* child) override;
};

}  // namespace tb

#endif  // TB_WIDGET_ANIMATION_H
