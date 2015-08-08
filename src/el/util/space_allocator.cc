/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/config.h"
#include "el/util/space_allocator.h"

namespace el {
namespace util {

bool SpaceAllocator::HasSpace(int needed_w) const {
  if (needed_w > m_available_space) {
    return false;
  }
  if (empty()) {
    return true;
  }
  for (Space* fs = m_free_space_list.GetFirst(); fs; fs = fs->GetNext()) {
    if (needed_w <= fs->width) {
      return true;
    }
  }
  return false;
}

SpaceAllocator::Space* SpaceAllocator::AllocSpace(int needed_w) {
  if (Space* available_space = GetSmallestAvailableSpace(needed_w)) {
    Space* new_space = new Space();
    new_space->x = available_space->x;
    new_space->width = needed_w;
    m_used_space_list.AddLast(new_space);

    // Consume the used space from the available space.
    available_space->x += needed_w;
    available_space->width -= needed_w;
    m_available_space -= needed_w;

    // Remove it if empty.
    if (!available_space->width) {
      m_free_space_list.Delete(available_space);
    }
    return new_space;
  }
  return nullptr;
}

SpaceAllocator::Space* SpaceAllocator::GetSmallestAvailableSpace(int needed_w) {
  assert(needed_w > 0);

  // Add free space covering all available space if empty.
  if (!m_free_space_list.HasLinks() && empty()) {
    Space* fs = new Space();
    fs->x = 0;
    fs->width = m_available_space;
    m_free_space_list.AddLast(fs);
  }

  // Check for the smallest space where we fit.
  Space* best_fs = nullptr;
  for (Space* fs = m_free_space_list.GetFirst(); fs; fs = fs->GetNext()) {
    if (needed_w == fs->width) {
      return fs;  // It can't be better than a perfect match!
    }
    if (needed_w < fs->width) {
      if (!best_fs || fs->width < best_fs->width) {
        best_fs = fs;
      }
    }
  }
  return best_fs;
}

void SpaceAllocator::FreeSpace(Space* space) {
  m_used_space_list.Remove(space);
  m_available_space += space->width;

  // Find where in m_free_space_list we should insert the space,
  // or which existing space we can extend.
  Space* preceeding = nullptr;
  Space* succeeding = nullptr;
  for (Space* fs = m_free_space_list.GetFirst(); fs; fs = fs->GetNext()) {
    if (fs->x < space->x) {
      preceeding = fs;
    }
    if (fs->x > space->x) {
      succeeding = fs;
      break;
    }
  }
  if (preceeding && preceeding->x + preceeding->width == space->x) {
    preceeding->width += space->width;
    delete space;
  } else if (succeeding && succeeding->x == space->x + space->width) {
    succeeding->x -= space->width;
    succeeding->width += space->width;
    delete space;
  } else {
    if (preceeding) {
      m_free_space_list.AddAfter(space, preceeding);
    } else if (succeeding) {
      m_free_space_list.AddBefore(space, succeeding);
    } else {
      assert(!m_free_space_list.HasLinks());
      m_free_space_list.AddLast(space);
    }
  }
  // Merge free spaces.
  Space* fs = m_free_space_list.GetFirst();
  while (fs) {
    Space* next_fs = fs->GetNext();
    if (!next_fs) {
      break;
    }
    if (fs->x + fs->width == next_fs->x) {
      fs->width += next_fs->width;
      m_free_space_list.Delete(next_fs);
      continue;
    }
    fs = next_fs;
  }

#ifdef EL_RUNTIME_DEBUG_INFO
  // Check that free space is in order.
  Space* tmp = m_free_space_list.GetFirst();
  int x = 0;
  while (tmp) {
    assert(tmp->x >= x);
    x = tmp->x + tmp->width;
    tmp = tmp->GetNext();
  }
#endif  // EL_RUNTIME_DEBUG_INFO
}

}  // namespace util
}  // namespace el
