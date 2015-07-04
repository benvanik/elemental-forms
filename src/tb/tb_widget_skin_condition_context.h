/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_WIDGET_SKIN_CONDITION_CONTEXT_H
#define TB_WIDGET_SKIN_CONDITION_CONTEXT_H

#include "tb/resources/skin.h"
#include "tb_widgets.h"

namespace tb {

// Check if a condition is true for a element when painting a skin.
class ElementSkinConditionContext : public resources::SkinConditionContext {
 public:
  ElementSkinConditionContext(Element* element) : m_element(element) {}
  bool GetCondition(
      resources::SkinTarget target,
      const resources::SkinCondition::ConditionInfo& info) override;

 private:
  bool GetCondition(Element* element,
                    const resources::SkinCondition::ConditionInfo& info);
  Element* m_element = nullptr;
};

}  // namespace tb

#endif  // TB_WIDGET_SKIN_CONDITION_CONTEXT_H
