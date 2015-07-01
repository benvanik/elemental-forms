/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_MENU_WINDOW_H
#define TB_MENU_WINDOW_H

#include "tb_popup_window.h"
#include "tb_select.h"

namespace tb {

/** TBMenuWindow is a popup window that shows a list of items (TBSelectList).

        When selected it will invoke a click with the id given to the menu,
        and the id of the clicked item as ref_id, and then close itself.

        It may open sub items as new windows at the same time as this window is
   open.*/

class TBMenuWindow : public TBPopupWindow {
 public:
  // For safe typecasting
  TBOBJECT_SUBCLASS(TBMenuWindow, TBPopupWindow);

  TBMenuWindow(TBWidget* target, TBID id);
  ~TBMenuWindow();

  bool Show(TBSelectItemSource* source, const TBPopupAlignment& alignment,
            int initial_value = -1);

  TBSelectList* GetList() { return &m_select_list; }

  virtual bool OnEvent(const TBWidgetEvent& ev);

 private:
  TBSelectList m_select_list;
};

}  // namespace tb

#endif  // TB_MENU_WINDOW_H
