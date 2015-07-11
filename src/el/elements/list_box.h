/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_LIST_BOX_H_
#define EL_ELEMENTS_LIST_BOX_H_

#include "el/element.h"
#include "el/elements/layout_box.h"
#include "el/elements/scroll_container.h"
#include "el/list_item.h"

namespace el {
namespace elements {

// Shows a scrollable list of items provided by a ListItemSource.
class ListBox : public Element, public ListItemObserver {
 public:
  TBOBJECT_SUBCLASS(ListBox, Element);
  static void RegisterInflater();

  ListBox();
  ~ListBox() override;

  // Gets the default item source for this element.
  // This source can be used to add items of type GenericStringItem to this
  // element.
  // It is the item source that is fed from resource files.
  // If you need to add other types of items, or if you want to share item
  // sources between several DropDownButton/ListBox elements, use SetSource
  // using a external item source.
  GenericStringItemSource* default_source() { return &m_default_source; }

  const std::string& filter() const { return m_filter; }
  // Sets filter string so only matching items will be showed.
  // Set nullptr or empty string to remove filter and show all items.
  void set_filter(const char* filter);
  void set_filter(const std::string& filter) { set_filter(filter.c_str()); }

  // Sets the language string id for the header.
  // The header is shown at the top of the list when only a subset of all items
  // are shown.
  void set_header_string(const TBID& id);

  // Makes the list update its items to reflect the items from the in the
  // current source. The update will take place next time the list is validated.
  void InvalidateList();

  // Makes sure the list is reflecting the current items in the source.
  void ValidateList();

  int value() override { return m_value; }
  // Sets the value that is the selected item.
  // In lists with multiple selectable items it's the item that is the current
  // focus.
  void set_value(int value) override;

  // Gets the ID of the selected item, or 0 if there is no item selected.
  TBID selected_item_id();

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
  void ListItem(size_t index, bool selected);
  Element* GetItemElement(size_t index);

  // Scrolls to the current selected item. The scroll may be delayed until
  // the items has been layouted if the layout is currently invalid.
  void ScrollToSelectedItem();

  // Returns the scrollcontainer used in this list.
  ScrollContainer* scroll_container() { return &m_container; }

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnSkinChanged() override;
  void OnProcess() override;
  void OnProcessAfterChildren() override;
  bool OnEvent(const Event& ev) override;

  void OnSourceChanged() override;
  void OnItemChanged(size_t index) override;
  void OnItemAdded(size_t index) override;
  void OnItemRemoved(size_t index) override;
  void OnAllItemsRemoved() override;

 protected:
  ScrollContainer m_container;
  LayoutBox m_layout;
  GenericStringItemSource m_default_source;
  int m_value = -1;
  std::string m_filter;
  bool m_list_is_invalid = false;
  bool m_scroll_to_current = false;
  TBID m_header_lng_string_id;

 private:
  Element* CreateAndAddItemAfter(size_t index, Element* reference);
};

}  // namespace elements
namespace dsl {

struct ListBoxNode : public ItemListElementNode<ListBoxNode> {
  using R = ListBoxNode;
  ListBoxNode() : ItemListElementNode("ListBox") {}
  //
  R& value(int32_t value) {
    set("value", value);
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_LIST_BOX_H_
