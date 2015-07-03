/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_widget_skin_condition_context.h"

#include "tb_tab_container.h"
#include "tb_widgets_common.h"
#include "tb_window.h"

namespace tb {

bool WidgetSkinConditionContext::GetCondition(
    SkinTarget target, const SkinCondition::ConditionInfo& info) {
  switch (target) {
    case SkinTarget::kThis:
      return GetCondition(m_widget, info);
    case SkinTarget::kParent:
      return m_widget->GetParent() && GetCondition(m_widget->GetParent(), info);
    case SkinTarget::kAncestors: {
      Widget* widget = m_widget->GetParent();
      while (widget) {
        if (GetCondition(widget, info)) {
          return true;
        }
        widget = widget->GetParent();
      }
    }
    case SkinTarget::kPrevSibling:
      return m_widget->GetPrev() && GetCondition(m_widget->GetPrev(), info);
    case SkinTarget::kNextSibling:
      return m_widget->GetNext() && GetCondition(m_widget->GetNext(), info);
  }
  return false;
}

bool WidgetSkinConditionContext::GetCondition(
    Widget* widget, const SkinCondition::ConditionInfo& info) {
  switch (info.prop) {
    case SkinProperty::kSkin:
      return widget->GetSkinBg() == info.value;
    case SkinProperty::kWindowActive:
      if (Window* window = widget->GetParentWindow()) {
        return window->IsActive();
      }
      return false;
    case SkinProperty::kAxis:
      return TBID(widget->GetAxis() == Axis::kX ? "x" : "y") == info.value;
    case SkinProperty::kAlign:
      if (TabContainer* tc = TBSafeCast<TabContainer>(widget)) {
        TBID widget_align;
        if (tc->GetAlignment() == Align::kLeft) {
          widget_align = TBIDC("left");
        } else if (tc->GetAlignment() == Align::kTop) {
          widget_align = TBIDC("top");
        } else if (tc->GetAlignment() == Align::kRight) {
          widget_align = TBIDC("right");
        } else if (tc->GetAlignment() == Align::kBottom) {
          widget_align = TBIDC("bottom");
        }
        return widget_align == info.value;
      }
      return false;
    case SkinProperty::kId:
      return widget->GetID() == info.value;
    case SkinProperty::kState:
      return !!(uint32_t(widget->GetAutoState()) & info.value);
    case SkinProperty::kValue:
      return widget->GetValue() == (int)info.value;
    case SkinProperty::kHover:
      return Widget::hovered_widget &&
             widget->IsAncestorOf(Widget::hovered_widget);
    case SkinProperty::kCapture:
      return Widget::captured_widget &&
             widget->IsAncestorOf(Widget::captured_widget);
    case SkinProperty::kFocus:
      return Widget::focused_widget &&
             widget->IsAncestorOf(Widget::focused_widget);
    case SkinProperty::kCustom:
      return widget->GetCustomSkinCondition(info);
  }
  return false;
}

}  // namespace tb
