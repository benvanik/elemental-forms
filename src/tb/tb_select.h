/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_SELECT_H
#define TB_SELECT_H

#include "tb_scroll_container.h"
#include "tb_select_item.h"
#include "tb_window.h"

namespace tb {

class MenuWindow;

// Shows a scrollable list of items provided by a SelectItemSource.
class SelectList : public Element, public SelectItemObserver {
 public:
  TBOBJECT_SUBCLASS(SelectList, Element);

  SelectList();
  ~SelectList() override;

  // Gets the default item source for this element.
  // This source can be used to add items of type GenericStringItem to this
  // element.
  // It is the item source that is fed from resource files.
  // If you need to add other types of items, or if you want to share item
  // sources between several SelectDropdown/SelectList elements, use SetSource
  // using a external item source.
  GenericStringItemSource* GetDefaultSource() { return &m_default_source; }

  // Sets filter string so only matching items will be showed.
  // Set nullptr or empty string to remove filter and show all items.
  void SetFilter(const char* filter);
  void SetFilter(const std::string& filter) { SetFilter(filter.c_str()); }
  const std::string& GetFilter() const { return m_filter; }

  // Sets the language string id for the header.
  // The header is shown at the top of the list when only a subset of all items
  // are shown.
  void SetHeaderString(const TBID& id);

  // Makes the list update its items to reflect the items from the in the
  // current source. The update will take place next time the list is validated.
  void InvalidateList();

  // Makes sure the list is reflecting the current items in the source.
  void ValidateList();

  // Sets the value that is the selected item.
  // In lists with multiple selectable items it's the item that is the current
  // focus.
  void SetValue(int value) override;
  int GetValue() override { return m_value; }

  // Gets the ID of the selected item, or 0 if there is no item selected.
  TBID GetSelectedItemID();

  // Changes the value to a non disabled item that is visible with the current
  // filter.
  // Returns true if it successfully found another item.
  // Valid keys:
  //     SpecialKey::kUp   - Previous item.
  //     SpecialKey::kDown - Next item.
  //     SpecialKey::kHome - First item.
  //     SpecialKey::kEnd  - Last item.
  bool ChangeValue(SpecialKey key);

  // Sets the selected state of the item at the given index. If you want to
  // unselect the previously selected item, use SetValue.
  void SelectItem(size_t index, bool selected);
  Element* GetItemElement(size_t index);

  // Scrolls to the current selected item. The scroll may be delayed until
  // the items has been layouted if the layout is currently invalid.
  void ScrollToSelectedItem();

  // Returns the scrollcontainer used in this list.
  ScrollContainer* GetScrollContainer() { return &m_container; }

  void OnInflate(const InflateInfo& info) override;
  void OnSkinChanged() override;
  void OnProcess() override;
  void OnProcessAfterChildren() override;
  bool OnEvent(const ElementEvent& ev) override;

  void OnSourceChanged() override;
  void OnItemChanged(size_t index) override;
  void OnItemAdded(size_t index) override;
  void OnItemRemoved(size_t index) override;
  void OnAllItemsRemoved() override;

 protected:
  ScrollContainer m_container;
  Layout m_layout;
  GenericStringItemSource m_default_source;
  int m_value = -1;
  std::string m_filter;
  bool m_list_is_invalid = false;
  bool m_scroll_to_current = false;
  TBID m_header_lng_string_id;

 private:
  Element* CreateAndAddItemAfter(size_t index, Element* reference);
};

// Shows a button that opens a popup with a SelectList with items provided by a
// SelectItemSource.
class SelectDropdown : public Button, public SelectItemObserver {
 public:
  TBOBJECT_SUBCLASS(SelectDropdown, Button);

  SelectDropdown();
  ~SelectDropdown() override;

  // Gets the default item source for this element.
  // This source can be used to add items of type GenericStringItem to this
  // element.
  // It is the item source that is fed from resource files.
  // If you need to add other types of items, or if you want to share item
  // sources between several SelectDropdown/SelectList elements, use SetSource
  // using a external item source.
  GenericStringItemSource* GetDefaultSource() { return &m_default_source; }

  // Sets the selected item.
  void SetValue(int value) override;
  int GetValue() override { return m_value; }

  // Gets the ID of the selected item, or 0 if there is no item selected.
  TBID GetSelectedItemID();

  // Opens the window if the model has items.
  void OpenWindow();

  // Closes the window if it is open.
  void CloseWindow();

  // Returns the menu window if it's open, or nullptr.
  MenuWindow* GetMenuIfOpen() const;

  void OnInflate(const InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;

  void OnSourceChanged() override;
  void OnItemChanged(size_t index) override;
  void OnItemAdded(size_t index) override {}
  void OnItemRemoved(size_t index) override {}
  void OnAllItemsRemoved() override {}

 protected:
  GenericStringItemSource m_default_source;
  SkinImage m_arrow;
  int m_value = -1;
  WeakElementPointer m_window_pointer;  // Dropdown window, if opened.
};

}  // namespace tb

#endif  // TB_SELECT_H
