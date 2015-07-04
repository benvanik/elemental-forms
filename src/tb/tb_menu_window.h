/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_MENU_WINDOW_H
#define TB_MENU_WINDOW_H

#include "tb_popup_window.h"
#include "tb_select.h"

namespace tb {

// MenuWindow is a popup window that shows a list of items (SelectList).
// When selected it will invoke a click with the id given to the menu,
// and the id of the clicked item as ref_id, and then close itself.
// It may open sub items as new windows at the same time as this window is open.
class MenuWindow : public PopupWindow {
 public:
  TBOBJECT_SUBCLASS(MenuWindow, PopupWindow);

  MenuWindow(Element* target, TBID id);
  ~MenuWindow() override;

  bool Show(SelectItemSource* source, const PopupAlignment& alignment,
            int initial_value = -1);

  SelectList* GetList() { return &m_select_list; }

  bool OnEvent(const ElementEvent& ev) override;

 private:
  SelectList m_select_list;
};

}  // namespace tb

#endif  // TB_MENU_WINDOW_H
