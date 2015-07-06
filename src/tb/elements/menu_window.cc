/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/element_listener.h"
#include "tb/elements/menu_window.h"

namespace tb {
namespace elements {

MenuWindow::MenuWindow(Element* target, TBID id) : PopupWindow(target) {
  set_id(id);
  set_background_skin(TBIDC("MenuWindow"), InvokeInfo::kNoCallbacks);
  m_select_list.scroll_container()->set_adapt_to_content_size(true);
  // Avoid it autoclosing its window on click:
  m_select_list.set_focusable(false);
  m_select_list.set_background_skin("");
  m_select_list.set_rect(padding_rect());
  m_select_list.set_gravity(Gravity::kAll);
  AddChild(&m_select_list);
}

MenuWindow::~MenuWindow() { RemoveChild(&m_select_list); }

void MenuWindow::Show(ListItemSource* source, const PopupAlignment& alignment,
                      int initial_value) {
  m_select_list.set_value(initial_value);
  m_select_list.set_source(source);
  m_select_list.ValidateList();
  PopupWindow::Show(alignment);
}

bool MenuWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick && &m_select_list == ev.target) {
    WeakElementPointer this_element(this);

    // Invoke the click on the target.
    Event target_ev(EventType::kClick);
    target_ev.ref_id = ev.ref_id;
    InvokeEvent(target_ev);

    // If target got deleted, close.
    if (this_element.get()) {
      Close();
    }
    return true;
  }
  return PopupWindow::OnEvent(ev);
}

}  // namespace elements
}  // namespace tb
