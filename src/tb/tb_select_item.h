/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_SELECT_ITEM_H
#define TB_SELECT_ITEM_H

#include "tb_linklist.h"
#include "tb_list.h"
#include "tb_id.h"
#include "tb_value.h"

namespace tb {

class SelectItemSource;

enum class Sort {
  kNone,       // No sorting. Items appear in list order.
  kAscending,  // Ascending sort.
  kDescending  // Descending sort.
};

// An observer for items provided by SelectItemSource.
// There can be multiple observers for each source. The observer will recieve
// callbacks when the source is changed, so it can update itself.
class SelectItemObserver : public TBLinkOf<SelectItemObserver> {
 public:
  SelectItemObserver() = default;
  virtual ~SelectItemObserver() = default;

  // Sets the source which should provide the items for this observer.
  // This source needs to live longer than this observer.
  // Set nullptr to unset currently set source.
  void SetSource(SelectItemSource* source);
  SelectItemSource* GetSource() const { return m_source; }

  // Called when the source has changed or been unset by calling SetSource.
  virtual void OnSourceChanged() = 0;

  // Called when the item at the given index has changed in a way that should
  // update the observer.
  virtual void OnItemChanged(int index) = 0;

  // Called when the item at the given index has been added.
  virtual void OnItemAdded(int index) = 0;

  // Called when the item at the given index has been removed.
  virtual void OnItemRemoved(int index) = 0;

  // Called when all items have been removed.
  virtual void OnAllItemsRemoved() = 0;

 protected:
  SelectItemSource* m_source = nullptr;
};

/** SelectItemSource is a item provider interface for list widgets
   (SelectList and
        SelectDropdown).

        Instead of feeding all list widgets with all items all the time, the
   list widgets
        will ask SelectItemSource when it needs it. The list widgets may also
   apply
        filtering so only a subset of all the items are shown.

        CreateItemWidget can be overridden to create any set of widget content
   for each item.

        This class has no storage of items. If you want an array storage of
   items,
        use the subclass SelectItemSourceList. If you implement your own
   storage,
        remember to call InvokeItem[Added/...] to notify observers that they
   need
   to update.
*/

class SelectItemSource {
 public:
  SelectItemSource() = default;
  virtual ~SelectItemSource();

  // Returns true if a item matches the given filter text.
  // By default, it returns true if GetItemString contains filter.
  virtual bool Filter(int index, const std::string& filter);

  // Gets the string of a item.
  // If a item has more than one string, return the one that should be used for
  // inline-find (pressing keys in the list will scroll to the item starting
  // with the same letters), and for sorting the list.
  virtual const char* GetItemString(int index) = 0;

  // Gets the source to be used if this item should open a sub menu.
  virtual SelectItemSource* GetItemSubSource(int index) { return nullptr; }

  // Gets the skin image to be painted before the text for this item.
  virtual TBID GetItemImage(int index) { return TBID(); }

  // Get the ID of the item.
  virtual TBID GetItemID(int index) { return TBID(); }

  // Creates the item representation widget(s).
  // By default, it will create a Label for string-only items, and other
  // types for items that also has image or submenu.
  virtual TBWidget* CreateItemWidget(int index, SelectItemObserver* observer);

  // Gets the number of items.
  virtual int GetNumItems() = 0;

  // Sets sort type. Default is Sort::kNone.
  void SetSort(Sort sort) { m_sort = sort; }
  Sort GetSort() const { return m_sort; }

  // Invokes OnItemChanged on all open observers for this source.
  void InvokeItemChanged(int index,
                         SelectItemObserver* exclude_observer = nullptr);
  void InvokeItemAdded(int index);
  void InvokeItemRemoved(int index);
  void InvokeAllItemsRemoved();

 private:
  friend class SelectItemObserver;
  TBLinkListOf<SelectItemObserver> m_observers;
  Sort m_sort = Sort::kNone;
};

// An item provider for list widgets (SelectList and SelectDropdown).
// It stores items of the type specified by the template in an array.
template <class T>
class SelectItemSourceList : public SelectItemSource {
 public:
  SelectItemSourceList() = default;
  ~SelectItemSourceList() override { DeleteAllItems(); }
  const char* GetItemString(int index) override {
    return GetItem(index)->str.c_str();
  }
  SelectItemSource* GetItemSubSource(int index) override {
    return GetItem(index)->sub_source;
  }
  TBID GetItemImage(int index) override { return GetItem(index)->skin_image; }
  TBID GetItemID(int index) override { return GetItem(index)->id; }
  int GetNumItems() override { return m_items.GetNumItems(); }
  TBWidget* CreateItemWidget(int index, SelectItemObserver* observer) override {
    if (TBWidget* widget =
            SelectItemSource::CreateItemWidget(index, observer)) {
      T* item = m_items[index];
      widget->SetID(item->id);
      return widget;
    }
    return nullptr;
  }

  // Adds a new item at the given index.
  void AddItem(T* item, int index) {
    m_items.Add(item, index);
    InvokeItemAdded(index);
  }

  // Adds a new item list.
  void AddItem(T* item) { AddItem(item, m_items.GetNumItems()); }

  // Gets the item at the given index.
  T* GetItem(int index) { return m_items[index]; }

  // Deletes the item at the given index.
  void DeleteItem(int index) {
    if (!m_items.GetNumItems()) return;
    m_items.Delete(index);
    InvokeItemRemoved(index);
  }

  // Deletes all items.
  void DeleteAllItems() {
    if (!m_items.GetNumItems()) return;
    m_items.DeleteAll();
    InvokeAllItemsRemoved();
  }

 private:
  TBListOf<T> m_items;
};

// An item for GenericStringItemSource.
// It has a string and may have a skin image and sub item source.
class GenericStringItem {
 public:
  GenericStringItem(const GenericStringItem& other)
      : str(other.str),
        id(other.id),
        sub_source(other.sub_source),
        tag(other.tag) {}
  GenericStringItem(const char* str) : str(str) {}
  GenericStringItem(const char* str, TBID id) : str(str), id(id) {}
  GenericStringItem(const char* str, SelectItemSource* sub_source)
      : str(str), sub_source(sub_source) {}
  GenericStringItem(const std::string& str) : str(str) {}
  GenericStringItem(const std::string& str, TBID id) : str(str), id(id) {}
  GenericStringItem(const std::string& str, SelectItemSource* sub_source)
      : str(str), sub_source(sub_source) {}
  const GenericStringItem& operator=(const GenericStringItem& other) {
    str = other.str;
    id = other.id;
    sub_source = other.sub_source;
    tag = other.tag;
    return *this;
  }

  void SetSkinImage(const TBID& image) { skin_image = image; }

 public:
  std::string str;
  TBID id;
  TBID skin_image;
  SelectItemSource* sub_source = nullptr;

  // This value is free to use for anything. It's not used internally.
  TBValue tag;
};

// An item source list providing items of type GenericStringItem.
class GenericStringItemSource : public SelectItemSourceList<GenericStringItem> {
};

}  // namespace tb

#endif  // TB_SELECT_ITEM_H
