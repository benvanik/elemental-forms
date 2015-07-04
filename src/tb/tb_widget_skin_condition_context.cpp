/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_widget_skin_condition_context.h"

#include "tb_tab_container.h"
#include "tb_widgets_common.h"
#include "tb_window.h"

namespace tb {

bool ElementSkinConditionContext::GetCondition(
    SkinTarget target, const SkinCondition::ConditionInfo& info) {
  switch (target) {
    case SkinTarget::kThis:
      return GetCondition(m_element, info);
    case SkinTarget::kParent:
      return m_element->GetParent() &&
             GetCondition(m_element->GetParent(), info);
    case SkinTarget::kAncestors: {
      Element* element = m_element->GetParent();
      while (element) {
        if (GetCondition(element, info)) {
          return true;
        }
        element = element->GetParent();
      }
    }
    case SkinTarget::kPrevSibling:
      return m_element->GetPrev() && GetCondition(m_element->GetPrev(), info);
    case SkinTarget::kNextSibling:
      return m_element->GetNext() && GetCondition(m_element->GetNext(), info);
  }
  return false;
}

bool ElementSkinConditionContext::GetCondition(
    Element* element, const SkinCondition::ConditionInfo& info) {
  switch (info.prop) {
    case SkinProperty::kSkin:
      return element->GetSkinBg() == info.value;
    case SkinProperty::kWindowActive:
      if (Window* window = element->GetParentWindow()) {
        return window->IsActive();
      }
      return false;
    case SkinProperty::kAxis:
      return TBID(element->GetAxis() == Axis::kX ? "x" : "y") == info.value;
    case SkinProperty::kAlign:
      if (TabContainer* tc = util::SafeCast<TabContainer>(element)) {
        TBID element_align;
        if (tc->GetAlignment() == Align::kLeft) {
          element_align = TBIDC("left");
        } else if (tc->GetAlignment() == Align::kTop) {
          element_align = TBIDC("top");
        } else if (tc->GetAlignment() == Align::kRight) {
          element_align = TBIDC("right");
        } else if (tc->GetAlignment() == Align::kBottom) {
          element_align = TBIDC("bottom");
        }
        return element_align == info.value;
      }
      return false;
    case SkinProperty::kId:
      return element->GetID() == info.value;
    case SkinProperty::kState:
      return !!(uint32_t(element->GetAutoState()) & info.value);
    case SkinProperty::kValue:
      return element->GetValue() == (int)info.value;
    case SkinProperty::kHover:
      return Element::hovered_element &&
             element->IsAncestorOf(Element::hovered_element);
    case SkinProperty::kCapture:
      return Element::captured_element &&
             element->IsAncestorOf(Element::captured_element);
    case SkinProperty::kFocus:
      return Element::focused_element &&
             element->IsAncestorOf(Element::focused_element);
    case SkinProperty::kCustom:
      return element->GetCustomSkinCondition(info);
  }
  return false;
}

}  // namespace tb
