/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/util/intrusive_list.h"

namespace el {
namespace util {
namespace impl {

IntrusiveListIterator::IntrusiveListIterator(IntrusiveList* linklist,
                                             IntrusiveListEntry* current_link,
                                             bool forward)
    : list_(linklist), current_entry_(current_link), iterate_forward_(forward) {
  Register();
}

IntrusiveListIterator::IntrusiveListIterator(const IntrusiveListIterator& iter)
    : list_(iter.list_),
      current_entry_(iter.current_entry_),
      iterate_forward_(iter.iterate_forward_) {
  Register();
}

IntrusiveListIterator::~IntrusiveListIterator() { Unregister(); }

void IntrusiveListIterator::Register() {
  prev_iterator_ = nullptr;
  next_iterator_ = list_->first_iterator;
  if (list_->first_iterator) {
    list_->first_iterator->prev_iterator_ = this;
  }
  list_->first_iterator = this;
}

void IntrusiveListIterator::Unregister() {
  if (!list_) {
    // Already unregistered
    return;
  }
  if (prev_iterator_) {
    prev_iterator_->next_iterator_ = next_iterator_;
  }
  if (next_iterator_) {
    next_iterator_->prev_iterator_ = prev_iterator_;
  }
  if (list_->first_iterator == this) {
    list_->first_iterator = next_iterator_;
  }
}

void IntrusiveListIterator::UnregisterAndClear() {
  Unregister();
  list_ = nullptr;
  current_entry_ = nullptr;
  prev_iterator_ = nullptr;
  next_iterator_ = nullptr;
}

const IntrusiveListIterator& IntrusiveListIterator::operator=(
    const IntrusiveListIterator& iter) {
  if (list_ != iter.list_) {
    // Change where we are registered if we change linklist.
    Unregister();
    list_ = iter.list_;
    Register();
  }
  list_ = iter.list_;
  current_entry_ = iter.current_entry_;
  iterate_forward_ = iter.iterate_forward_;
  return *this;
}

void IntrusiveListIterator::Reset() {
  if (list_) {
    current_entry_ = iterate_forward_ ? list_->first : list_->last;
  } else {
    current_entry_ = nullptr;
  }
}

IntrusiveListEntry* IntrusiveListIterator::GetAndStep() {
  if (!current_entry_) return nullptr;
  IntrusiveListEntry* current = current_entry_;
  current_entry_ =
      iterate_forward_ ? current_entry_->next : current_entry_->prev;
  return current;
}

void IntrusiveListIterator::RemoveLink(IntrusiveListEntry* link) {
  // If the current link is being removed, step away from it
  if (current_entry_ == link) {
    GetAndStep();
  }
}

IntrusiveList::~IntrusiveList() {
  RemoveAll();

  // Make sure any live iterators for this linklist are cleared!
  while (first_iterator) {
    first_iterator->UnregisterAndClear();
  }
}

void IntrusiveList::AddFirst(IntrusiveListEntry* link) {
  assert(!link->linklist);  // Link is already in some list!
  link->linklist = this;
  link->next = first;
  if (first) first->prev = link;
  first = link;
  if (!last) last = link;
}

void IntrusiveList::AddLast(IntrusiveListEntry* link) {
  assert(!link->linklist);  // Link is already in some list!
  link->linklist = this;
  link->prev = last;
  if (last) last->next = link;
  last = link;
  if (!first) first = link;
}

void IntrusiveList::AddBefore(IntrusiveListEntry* link,
                              IntrusiveListEntry* reference) {
  assert(reference->linklist == this);  // Reference is not added to this list!
  link->linklist = this;
  link->prev = reference->prev;
  link->next = reference;
  if (reference->prev) {
    reference->prev->next = link;
  } else {
    first = link;
  }
  reference->prev = link;
}

void IntrusiveList::AddAfter(IntrusiveListEntry* link,
                             IntrusiveListEntry* reference) {
  assert(reference->linklist == this);  // Reference is not added to this list!
  link->linklist = this;
  link->prev = reference;
  link->next = reference->next;
  if (reference->next) {
    reference->next->prev = link;
  } else {
    last = link;
  }
  reference->next = link;
}

void IntrusiveList::Remove(IntrusiveListEntry* link) {
  assert(link->linklist == this);  // Link is not added to this list!

  // Go through iterators and make sure there are no pointers
  // to the link we remove.
  IntrusiveListIterator* iter = first_iterator;
  while (iter) {
    iter->RemoveLink(link);
    iter = iter->next_iterator_;
  }
  // Remove the link
  if (link->next) {
    link->next->prev = link->prev;
  }
  if (link->prev) {
    link->prev->next = link->next;
  }
  if (first == link) {
    first = link->next;
  }
  if (last == link) {
    last = link->prev;
  }
  link->linklist = nullptr;
  link->prev = nullptr;
  link->next = nullptr;
}

void IntrusiveList::RemoveAll() {
  // Reset all iterators.
  IntrusiveListIterator* iter = first_iterator;
  while (iter) {
    iter->current_entry_ = nullptr;
    iter = iter->next_iterator_;
  }
  // Remove all links
  IntrusiveListEntry* link = first;
  while (link) {
    IntrusiveListEntry* next = link->next;
    link->linklist = nullptr;
    link->prev = nullptr;
    link->next = nullptr;
    link = next;
  }
  first = nullptr;
  last = nullptr;
}

int IntrusiveList::CountLinks() const {
  int count = 0;
  for (IntrusiveListEntry* link = first; link; link = link->next) {
    count++;
  }
  return count;
}

}  // namespace impl
}  // namespace util
}  // namespace el
