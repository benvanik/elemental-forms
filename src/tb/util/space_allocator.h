/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_SPACE_ALLOCATOR_H_
#define TB_UTIL_SPACE_ALLOCATOR_H_

#include "tb/util/link_list.h"

namespace tb {
namespace util {

// Allocates of space out of a given available space.
class SpaceAllocator {
 public:
  struct Space : public TBLinkOf<Space> {
    int x;
    int width;
  };

  SpaceAllocator(int available_space) : m_available_space(available_space) {}

  // Returns true if no allocations are currently live using this allocator.
  bool IsAllAvailable() const { return !m_used_space_list.HasLinks(); }
  // Returns true if the given width is currently available.
  bool HasSpace(int needed_w) const;
  // Allocates the given space and return the Space, or nullptr on error.
  Space* AllocSpace(int needed_w);
  // Frees the given space so it is available for new allocations.
  void FreeSpace(Space* space);

 private:
  Space* GetSmallestAvailableSpace(int needed_w);

  int m_available_space;
  TBLinkListAutoDeleteOf<Space> m_free_space_list;
  TBLinkListAutoDeleteOf<Space> m_used_space_list;
};

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_SPACE_ALLOCATOR_H_
