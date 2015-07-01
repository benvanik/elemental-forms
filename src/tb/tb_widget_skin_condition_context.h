/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WIDGET_SKIN_CONDITION_CONTEXT_H
#define TB_WIDGET_SKIN_CONDITION_CONTEXT_H

#include "tb_skin.h"
#include "tb_widgets.h"

namespace tb {

/** Check if a condition is true for a widget when painting a skin. */

class TBWidgetSkinConditionContext : public TBSkinConditionContext {
 public:
  TBWidgetSkinConditionContext(TBWidget* widget) : m_widget(widget) {}
  virtual bool GetCondition(TBSkinCondition::TARGET target,
                            const TBSkinCondition::CONDITION_INFO& info);

 private:
  bool GetCondition(TBWidget* widget,
                    const TBSkinCondition::CONDITION_INFO& info);
  TBWidget* m_widget;
};

}  // namespace tb

#endif  // TB_WIDGET_SKIN_CONDITION_CONTEXT_H
