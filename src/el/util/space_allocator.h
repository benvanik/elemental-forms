/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_SPACE_ALLOCATOR_H_
#define EL_UTIL_SPACE_ALLOCATOR_H_

#include "el/util/intrusive_list.h"

namespace el {
namespace util {

// Allocates of space out of a given available space.
class SpaceAllocator {
 public:
  struct Space : public IntrusiveListEntry<Space> {
    int x;
    int width;
  };

  SpaceAllocator(int available_space) : m_available_space(available_space) {}

  // Returns true if no allocations are currently live using this allocator.
  bool empty() const { return !m_used_space_list.HasLinks(); }
  // Returns true if the given width is currently available.
  bool HasSpace(int needed_w) const;
  // Allocates the given space and return the Space, or nullptr on error.
  Space* AllocSpace(int needed_w);
  // Frees the given space so it is available for new allocations.
  void FreeSpace(Space* space);

 private:
  Space* GetSmallestAvailableSpace(int needed_w);

  int m_available_space;
  AutoDeleteIntrusiveList<Space> m_free_space_list;
  AutoDeleteIntrusiveList<Space> m_used_space_list;
};

}  // namespace util
}  // namespace el

#endif  // EL_UTIL_SPACE_ALLOCATOR_H_
