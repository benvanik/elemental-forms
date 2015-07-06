/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_DROP_DOWN_BUTTON_H_
#define TB_ELEMENTS_DROP_DOWN_BUTTON_H_

#include "tb/element.h"
#include "tb/elements/icon_box.h"
#include "tb/list_item.h"
#include "tb/window.h"

namespace tb {
namespace elements {

class MenuWindow;

// Shows a button that opens a popup with a ListBox with items provided by a
// ItemSource.
class DropDownButton : public Button, public ListItemObserver {
 public:
  TBOBJECT_SUBCLASS(DropDownButton, Button);
  static void RegisterInflater();

  DropDownButton();
  ~DropDownButton() override;

  // Gets the default item source for this element.
  // This source can be used to add items of type GenericStringItem to this
  // element.
  // It is the item source that is fed from resource files.
  // If you need to add other types of items, or if you want to share item
  // sources between several DropDownButton/ListBox elements, use SetSource
  // using a external item source.
  GenericStringItemSource* default_source() { return &m_default_source; }

  int value() override { return m_value; }
  // Sets the selected item.
  void set_value(int value) override;

  // Gets the ID of the selected item, or 0 if there is no item selected.
  TBID selected_item_id();

  // Opens the window if the model has items.
  void OpenWindow();

  // Closes the window if it is open.
  void CloseWindow();

  // Returns the menu window if it's open, or nullptr.
  MenuWindow* menu_if_open() const;

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const Event& ev) override;

  void OnSourceChanged() override;
  void OnItemChanged(size_t index) override;
  void OnItemAdded(size_t index) override {}
  void OnItemRemoved(size_t index) override {}
  void OnAllItemsRemoved() override {}

 protected:
  GenericStringItemSource m_default_source;
  IconBox m_arrow;
  int m_value = -1;
  WeakElementPointer m_window_pointer;  // Dropdown window, if opened.
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_DROP_DOWN_BUTTON_H_
