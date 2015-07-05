/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_LIST_ITEM_H_
#define TB_LIST_ITEM_H_

#include <memory>
#include <vector>

#include "tb/id.h"
#include "tb/util/intrusive_list.h"
#include "tb/value.h"

namespace tb {

class Element;
class ListItemSource;
namespace parsing {
class ParseNode;
}  // namespace parsing

enum class Sort {
  kNone,       // No sorting. Items appear in list order.
  kAscending,  // Ascending sort.
  kDescending  // Descending sort.
};

// An observer for items provided by ListItemSource.
// There can be multiple observers for each source. The observer will recieve
// callbacks when the source is changed, so it can update itself.
class ListItemObserver : public util::IntrusiveListEntry<ListItemObserver> {
 public:
  ListItemObserver() = default;
  virtual ~ListItemObserver() = default;

  // Sets the source which should provide the items for this observer.
  // This source needs to live longer than this observer.
  // Set nullptr to unset currently set source.
  void SetSource(ListItemSource* source);
  ListItemSource* GetSource() const { return m_source; }

  // Called when the source has changed or been unset by calling SetSource.
  virtual void OnSourceChanged() = 0;

  // Called when the item at the given index has changed in a way that should
  // update the observer.
  virtual void OnItemChanged(size_t index) = 0;

  // Called when the item at the given index has been added.
  virtual void OnItemAdded(size_t index) = 0;

  // Called when the item at the given index has been removed.
  virtual void OnItemRemoved(size_t index) = 0;

  // Called when all items have been removed.
  virtual void OnAllItemsRemoved() = 0;

 protected:
  ListItemSource* m_source = nullptr;
};

// An item provider interface for list elements (ListBox and DropDownButton).
// Instead of feeding all list elements with all items all the time, the list
// elements will ask ListItemSource when it needs it. The list elements may
// also apply filtering so only a subset of all the items are shown.
//
// CreateItemElement can be overridden to create any set of element content for
// each item.
//
// This class has no storage of items. If you want an array storage of items,
// use the subclass ListItemSourceList. If you implement your own storage,
// remember to call InvokeItem[Added/...] to notify observers that they need
// to update.
class ListItemSource {
 public:
  ListItemSource() = default;
  virtual ~ListItemSource();

  // Returns true if a item matches the given filter text.
  // By default, it returns true if GetItemString contains filter.
  virtual bool Filter(size_t index, const std::string& filter);

  // Gets the string of a item.
  // If a item has more than one string, return the one that should be used for
  // inline-find (pressing keys in the list will scroll to the item starting
  // with the same letters), and for sorting the list.
  virtual const char* GetItemString(size_t index) = 0;

  // Gets the source to be used if this item should open a sub menu.
  virtual ListItemSource* GetItemSubSource(size_t index) { return nullptr; }

  // Gets the skin image to be painted before the text for this item.
  virtual TBID GetItemImage(size_t index) { return TBID(); }

  // Get the ID of the item.
  virtual TBID GetItemID(size_t index) { return TBID(); }

  // Creates the item representation element(s).
  // By default, it will create a Label for string-only items, and other
  // types for items that also has image or submenu.
  virtual Element* CreateItemElement(size_t index, ListItemObserver* observer);

  // Gets the number of items.
  virtual size_t size() = 0;

  // Sets sort type. Default is Sort::kNone.
  void SetSort(Sort sort) { m_sort = sort; }
  Sort GetSort() const { return m_sort; }

  // Invokes OnItemChanged on all open observers for this source.
  void InvokeItemChanged(size_t index,
                         ListItemObserver* exclude_observer = nullptr);
  void InvokeItemAdded(size_t index);
  void InvokeItemRemoved(size_t index);
  void InvokeAllItemsRemoved();

 private:
  friend class ListItemObserver;
  util::IntrusiveList<ListItemObserver> m_observers;
  Sort m_sort = Sort::kNone;
};

// An item provider for list elements (ListBox and DropDownButton).
// It stores items of the type specified by the template in an array.
template <class T>
class ListItemSourceList : public ListItemSource {
 public:
  ListItemSourceList() = default;
  ~ListItemSourceList() override { DeleteAllItems(); }

  const char* GetItemString(size_t index) override {
    return GetItem(index)->str.c_str();
  }
  ListItemSource* GetItemSubSource(size_t index) override {
    return GetItem(index)->sub_source;
  }
  TBID GetItemImage(size_t index) override {
    return GetItem(index)->skin_image;
  }
  TBID GetItemID(size_t index) override { return GetItem(index)->id; }
  size_t size() override { return items_.size(); }

  Element* CreateItemElement(size_t index,
                             ListItemObserver* observer) override {
    if (Element* element = ListItemSource::CreateItemElement(index, observer)) {
      auto& item = items_[index];
      element->set_id(item->id);
      return element;
    }
    return nullptr;
  }

  // Adds a new item at the given index.
  void InsertItem(size_t index, std::unique_ptr<T> item) {
    items_.insert(index, std::move(item));
    InvokeItemAdded(index);
  }

  // Adds a new item list.
  void AddItem(std::unique_ptr<T> item) {
    items_.push_back(std::move(item));
    InvokeItemAdded(items_.size() - 1);
  }

  // Gets the item at the given index.
  T* GetItem(size_t index) { return items_[index].get(); }

  // Deletes the item at the given index.
  void DeleteItem(size_t index) {
    if (items_.empty()) {
      return;
    }
    items_.erase(items_.begin() + index);
    InvokeItemRemoved(index);
  }

  // Deletes all items.
  void DeleteAllItems() {
    if (items_.empty()) {
      return;
    }
    items_.clear();
    InvokeAllItemsRemoved();
  }

 private:
  std::vector<std::unique_ptr<T>> items_;
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
  GenericStringItem(const char* str, ListItemSource* sub_source)
      : str(str), sub_source(sub_source) {}
  GenericStringItem(const std::string& str) : str(str) {}
  GenericStringItem(const std::string& str, TBID id) : str(str), id(id) {}
  GenericStringItem(const std::string& str, ListItemSource* sub_source)
      : str(str), sub_source(sub_source) {}
  const GenericStringItem& operator=(const GenericStringItem& other) {
    str = other.str;
    id = other.id;
    sub_source = other.sub_source;
    tag = other.tag;
    return *this;
  }

  void SetIconBox(const TBID& image) { skin_image = image; }

 public:
  std::string str;
  TBID id;
  TBID skin_image;
  ListItemSource* sub_source = nullptr;

  // This value is free to use for anything. It's not used internally.
  Value tag;
};

// An item source list providing items of type GenericStringItem.
class GenericStringItemSource : public ListItemSourceList<GenericStringItem> {
 public:
  static void ReadItemNodes(tb::parsing::ParseNode* parent_node,
                            GenericStringItemSource* target_source);
};

}  // namespace tb

#endif  // TB_LIST_ITEM_H_
