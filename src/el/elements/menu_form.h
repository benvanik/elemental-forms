/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_MENU_FORM_H_
#define EL_ELEMENTS_MENU_FORM_H_

#include "el/elements/list_box.h"
#include "el/elements/popup_form.h"

namespace el {
namespace elements {

// MenuForm is a popup form that shows a list of items (ListBox).
// When selected it will invoke a click with the id given to the menu,
// and the id of the clicked item as ref_id, and then close itself.
// It may open sub items as new forms at the same time as this form is open.
class MenuForm : public PopupForm {
 public:
  TBOBJECT_SUBCLASS(MenuForm, PopupForm);

  MenuForm(Element* target, TBID id);
  ~MenuForm() override;

  void Show(ListItemSource* source, const PopupAlignment& alignment,
            int initial_value = -1);

  ListBox* list_box() { return &m_select_list; }

  bool OnEvent(const Event& ev) override;

 private:
  ListBox m_select_list;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_MENU_FORM_H_
