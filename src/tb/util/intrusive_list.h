/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_INTRUSIVE_LIST_H_
#define TB_UTIL_INTRUSIVE_LIST_H_

#include <cassert>

namespace tb {
namespace util {

namespace impl {

class IntrusiveList;

// The backend class to be inserted in IntrusiveList.
// Use the typed IntrusiveListEntry for object storing!
struct IntrusiveListEntry {
  // Returns true if the link is currently added to a list.
  bool IsInList() const { return linklist ? true : false; }

  IntrusiveListEntry* prev = nullptr;
  IntrusiveListEntry* next = nullptr;
  IntrusiveList* linklist = nullptr;
};

// The backend class for a safe iteration of an IntrusiveList.
// You would normally recieve a typed iterator from a
// IntrusiveList::IterateForward
// or IntrusiveList::IterateBackward, instead of creating this object
// directly.
//
// Safe iteration means that if a link is removed from a linked list, _all_
// iterators that currently
// point to that link will automatically step to the next link in the
// iterators direction.
class IntrusiveListIterator {
 public:
  IntrusiveListIterator(const IntrusiveListIterator& iter);
  IntrusiveListIterator(IntrusiveList* linklist,
                        IntrusiveListEntry* current_link, bool forward);
  ~IntrusiveListIterator();

  /** Set the iterator to the first link in we iterate forward,
  or set it to the last link if we iterate backward.  */
  void Reset();

  /** Get the current link or nullptr if out of bounds. */
  IntrusiveListEntry* get() const { return current_entry_; }

  /** Get the current link and step the iterator to the next (forward or
  * backward). */
  IntrusiveListEntry* GetAndStep();

  operator IntrusiveListEntry*() const { return current_entry_; }

  const IntrusiveListIterator& operator=(const IntrusiveListIterator& iter);

 private:
  /** RemoveLink is called when removing/deleting links in the target linklist.
  This will make sure iterators skip the deleted item. */
  void RemoveLink(IntrusiveListEntry* link);
  friend class IntrusiveList;

  /** Add ourself to the chain of iterators in the linklist. */
  void Register();

  /** Unlink ourself from the chain of iterators in the linklist. */
  void Unregister();
  void UnregisterAndClear();

  IntrusiveList* list_;                // The linklist we are iterating.
  IntrusiveListEntry* current_entry_;  // The current link, or nullptr.
  bool iterate_forward_;  // true if we iterate from first to last item.

  IntrusiveListIterator*
      prev_iterator_;  // Link in list of iterators for list_.
  IntrusiveListIterator*
      next_iterator_;  // Link in list of iterators for list_.
};

// The backend for IntrusiveListOf and IntrusiveListAutoDeleteOf.
// You should use the typed IntrusiveListOf or IntrusiveListAutoDeleteOf for
// object storing!
class IntrusiveList {
 public:
  IntrusiveList() : first(nullptr), last(nullptr), first_iterator(nullptr) {}
  ~IntrusiveList();

  void Remove(IntrusiveListEntry* link);
  void RemoveAll();

  void AddFirst(IntrusiveListEntry* link);
  void AddLast(IntrusiveListEntry* link);

  void AddBefore(IntrusiveListEntry* link, IntrusiveListEntry* reference);
  void AddAfter(IntrusiveListEntry* link, IntrusiveListEntry* reference);

  bool ContainsLink(IntrusiveListEntry* link) const {
    return link->linklist == this;
  }

  bool HasLinks() const { return first ? true : false; }

  int CountLinks() const;

 public:
  IntrusiveListEntry* first;
  IntrusiveListEntry* last;
  IntrusiveListIterator* first_iterator;
};

}  // namespace impl

template <typename T>
class IntrusiveListEntry : public impl::IntrusiveListEntry {
 public:
  inline T* GetPrev() const { return (T*)prev; }
  inline T* GetNext() const { return (T*)next; }
};

/** IntrusiveList is a double linked linklist. */
template <typename T>
class IntrusiveList {
 public:
  using TLink = IntrusiveListEntry<T>;

  /** Remove link from this linklist. */
  void Remove(T* link) { base_list_.Remove(static_cast<TLink*>(link)); }

  /** Remove all links without deleting them. */
  void RemoveAll() { base_list_.RemoveAll(); }

  /** Add link first in this linklist. */
  void AddFirst(T* link) { base_list_.AddFirst(static_cast<TLink*>(link)); }

  /** Add link last in this linklist. */
  void AddLast(T* link) { base_list_.AddLast(static_cast<TLink*>(link)); }

  /** Add link before the reference link (which must be added to this linklist).
  */
  void AddBefore(T* link, T* reference) {
    base_list_.AddBefore(static_cast<TLink*>(link), reference);
  }

  /** Add link after the reference link (which must be added to this linklist).
  */
  void AddAfter(T* link, T* reference) {
    base_list_.AddAfter(static_cast<TLink*>(link), reference);
  }

  /** Return true if the link is currently added to this linklist. */
  bool ContainsLink(T* link) const {
    return base_list_.ContainsLink(static_cast<TLink*>(link));
  }

  /** Get the first link, or nullptr. */
  T* GetFirst() const { return (T*)static_cast<TLink*>(base_list_.first); }

  /** Get the last link, or nullptr. */
  T* GetLast() const { return (T*)static_cast<TLink*>(base_list_.last); }

  /** Return true if this linklist contains any links. */
  bool HasLinks() const { return base_list_.HasLinks(); }

  /** Count the number of links in this list by iterating through all links. */
  int CountLinks() const { return base_list_.CountLinks(); }

  // Typed iterator for safe iteration. For more info, see
  // IntrusiveListIterator.
  class Iterator : public impl::IntrusiveListIterator {
   public:
    Iterator(IntrusiveList<T>* linklistof, bool forward)
        : impl::IntrusiveListIterator(&linklistof->base_list_,
                                      forward ? linklistof->base_list_.first
                                              : linklistof->base_list_.last,
                                      forward) {}
    Iterator(IntrusiveList<T>* linklistof, T* link, bool forward)
        : impl::IntrusiveListIterator(&linklistof->base_list_, link, forward) {}
    inline T* get() const {
      return (T*)static_cast<TLink*>(impl::IntrusiveListIterator::get());
    }
    inline T* GetAndStep() {
      return (T*)static_cast<TLink*>(impl::IntrusiveListIterator::GetAndStep());
    }
    inline operator T*() const { return (T*)static_cast<TLink*>(get()); }
  };

  /** Get a forward iterator that starts with the first link. */
  Iterator IterateForward() { return Iterator(this, true); }

  /** Get a forward iterator that starts with the given link. */
  Iterator IterateForward(T* link) { return Iterator(this, link, true); }

  /** Get a backward iterator that starts with the last link. */
  Iterator IterateBackward() { return Iterator(this, false); }

  /** Get a backward iterator that starts with the given link. */
  Iterator IterateBackward(T* link) { return Iterator(this, link, false); }

 private:
  impl::IntrusiveList base_list_;
};

// A double linked linklist that deletes all links on destruction.
template <typename T>
class AutoDeleteIntrusiveList : public IntrusiveList<T> {
 public:
  ~AutoDeleteIntrusiveList() { DeleteAll(); }

  // void Remove(T* link) = delete;
  void RemoveAll() = delete;

  // Removes a link from this list and deletes it.
  void Delete(T* link) {
    IntrusiveList<T>::Remove(link);
    delete link;
  }

  // Deletes all links in this linklist.
  void DeleteAll() {
    while (T* t = GetFirst()) {
      Delete(t);
    }
  }
};

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_INTRUSIVE_LIST_H_
