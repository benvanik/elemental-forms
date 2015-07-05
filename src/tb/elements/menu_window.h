/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_MENU_WINDOW_H_
#define TB_ELEMENTS_MENU_WINDOW_H_

#include "tb/elements/list_box.h"
#include "tb/elements/popup_window.h"

namespace tb {
namespace elements {

// MenuWindow is a popup window that shows a list of items (ListBox).
// When selected it will invoke a click with the id given to the menu,
// and the id of the clicked item as ref_id, and then close itself.
// It may open sub items as new windows at the same time as this window is open.
class MenuWindow : public PopupWindow {
 public:
  TBOBJECT_SUBCLASS(MenuWindow, PopupWindow);

  MenuWindow(Element* target, TBID id);
  ~MenuWindow() override;

  bool Show(ListItemSource* source, const PopupAlignment& alignment,
            int initial_value = -1);

  ListBox* list_box() { return &m_select_list; }

  bool OnEvent(const ElementEvent& ev) override;

 private:
  ListBox m_select_list;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_MENU_WINDOW_H_
