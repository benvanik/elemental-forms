/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_LIST_ITEM_H_
#define EL_LIST_ITEM_H_

#include <memory>
#include <vector>

#include "el/dsl.h"
#include "el/id.h"
#include "el/util/intrusive_list.h"
#include "el/value.h"

namespace el {

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

  ListItemSource* source() const { return m_source; }
  // Sets the source which should provide the items for this observer.
  // This source needs to live longer than this observer.
  // Set nullptr to unset currently set source.
  void set_source(ListItemSource* source);

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
  virtual TBID GetItemId(size_t index) { return TBID(); }

  // Creates the item representation element(s).
  // By default, it will create a Label for string-only items, and other
  // types for items that also has image or submenu.
  virtual Element* CreateItemElement(size_t index, ListItemObserver* observer);

  // Gets the number of items.
  virtual size_t size() = 0;

  Sort sort() const { return m_sort; }
  // Sets sort type. Default is Sort::kNone.
  void set_sort(Sort sort) { m_sort = sort; }

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
  ~ListItemSourceList() override { clear(); }

  const char* GetItemString(size_t index) override {
    return at(index)->str.c_str();
  }
  ListItemSource* GetItemSubSource(size_t index) override {
    return at(index)->sub_source;
  }
  TBID GetItemImage(size_t index) override { return at(index)->skin_image; }
  TBID GetItemId(size_t index) override { return at(index)->id; }
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
  void insert(size_t index, std::unique_ptr<T> item) {
    items_.insert(index, std::move(item));
    InvokeItemAdded(index);
  }

  // Adds a new item list.
  void push_back(std::unique_ptr<T> item) {
    items_.push_back(std::move(item));
    InvokeItemAdded(items_.size() - 1);
  }

  // Gets the item at the given index.
  T* at(size_t index) { return items_[index].get(); }

  T* operator[](size_t index) const { return items_[index].get(); }

  // Deletes the item at the given index.
  void erase(size_t index) {
    if (items_.empty()) {
      return;
    }
    items_.erase(items_.begin() + index);
    InvokeItemRemoved(index);
  }

  // Deletes all items.
  void clear() {
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

  void set_icon(const TBID& image) { skin_image = image; }

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
  static void ReadItemNodes(el::parsing::ParseNode* parent_node,
                            GenericStringItemSource* target_source);
};

namespace dsl {

struct Item {
  std::string id;
  std::string text;
  Item(std::string id, std::string text) : id(id), text(text) {}
};

template <typename T>
struct ItemListElementNode : public ElementNode<T> {
 protected:
  ItemListElementNode(const char* name) : ElementNode<T>(name) {}

 public:
  T& item(std::string text) {
    using parsing::ParseNode;
    auto items_node = Node::GetOrCreateNode("items");
    auto node = ParseNode::Create("item");
    auto text_node = ParseNode::Create("text");
    text_node->EmplaceValue(el::Value(text.c_str()));
    node->Add(text_node);
    items_node->Add(node);
    return *reinterpret_cast<T*>(this);
  }
  T& item(std::string id, std::string text) {
    using parsing::ParseNode;
    auto items_node = Node::GetOrCreateNode("items");
    auto node = ParseNode::Create("item");
    auto id_node = ParseNode::Create("id");
    id_node->EmplaceValue(el::Value(id.c_str()));
    node->Add(id_node);
    auto text_node = ParseNode::Create("text");
    text_node->EmplaceValue(el::Value(text.c_str()));
    node->Add(text_node);
    items_node->Add(node);
    return *reinterpret_cast<T*>(this);
  }
  T& item(int32_t id, std::string text) {
    using parsing::ParseNode;
    auto items_node = Node::GetOrCreateNode("items");
    auto node = ParseNode::Create("item");
    auto id_node = ParseNode::Create("id");
    id_node->EmplaceValue(el::Value(id));
    node->Add(id_node);
    auto text_node = ParseNode::Create("text");
    text_node->EmplaceValue(el::Value(text.c_str()));
    node->Add(text_node);
    items_node->Add(node);
    return *reinterpret_cast<T*>(this);
  }
  T& items(std::initializer_list<std::string> items) {
    using parsing::ParseNode;
    auto items_node = Node::GetOrCreateNode("items");
    for (auto& item : items) {
      auto node = ParseNode::Create("item");
      auto text_node = ParseNode::Create("text");
      text_node->EmplaceValue(el::Value(item.c_str()));
      node->Add(text_node);
      items_node->Add(node);
    }
    return *reinterpret_cast<T*>(this);
  }
  T& items(std::initializer_list<std::pair<int32_t, std::string>> items) {
    using parsing::ParseNode;
    auto items_node = Node::GetOrCreateNode("items");
    for (auto& item : items) {
      auto node = ParseNode::Create("item");
      auto id_node = ParseNode::Create("id");
      id_node->EmplaceValue(el::Value(item.first));
      node->Add(id_node);
      auto text_node = ParseNode::Create("text");
      text_node->EmplaceValue(el::Value(item.second.c_str()));
      node->Add(text_node);
      items_node->Add(node);
    }
    return *reinterpret_cast<T*>(this);
  }
  T& items(std::initializer_list<Item> items) {
    using parsing::ParseNode;
    auto items_node = Node::GetOrCreateNode("items");
    for (auto& item : items) {
      auto node = ParseNode::Create("item");
      auto id_node = ParseNode::Create("id");
      id_node->EmplaceValue(el::Value(item.id.c_str()));
      node->Add(id_node);
      auto text_node = ParseNode::Create("text");
      text_node->EmplaceValue(el::Value(item.text.c_str()));
      node->Add(text_node);
      items_node->Add(node);
    }
    return *reinterpret_cast<T*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_LIST_ITEM_H_
