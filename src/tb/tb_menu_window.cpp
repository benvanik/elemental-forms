/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_menu_window.h"

#include "tb_widgets_listener.h"

namespace tb {

MenuWindow::MenuWindow(Element* target, TBID id) : PopupWindow(target) {
  SetID(id);
  SetSkinBg(TBIDC("MenuWindow"), InvokeInfo::kNoCallbacks);
  m_select_list.GetScrollContainer()->SetAdaptToContentSize(true);
  // Avoid it autoclosing its window on click:
  m_select_list.SetIsFocusable(false);
  m_select_list.SetSkinBg("");
  m_select_list.set_rect(GetPaddingRect());
  m_select_list.SetGravity(Gravity::kAll);
  AddChild(&m_select_list);
}

MenuWindow::~MenuWindow() { RemoveChild(&m_select_list); }

bool MenuWindow::Show(SelectItemSource* source, const PopupAlignment& alignment,
                      int initial_value) {
  m_select_list.SetValue(initial_value);
  m_select_list.SetSource(source);
  m_select_list.ValidateList();
  return PopupWindow::Show(alignment);
}

bool MenuWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick && &m_select_list == ev.target) {
    WeakElementPointer this_element(this);

    // Invoke the click on the target.
    ElementEvent target_ev(EventType::kClick);
    target_ev.ref_id = ev.ref_id;
    InvokeEvent(target_ev);

    // If target got deleted, close.
    if (this_element.Get()) {
      Close();
    }
    return true;
  }
  return PopupWindow::OnEvent(ev);
}

}  // namespace tb
