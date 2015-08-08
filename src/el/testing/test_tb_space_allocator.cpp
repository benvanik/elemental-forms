/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/testing/testing.h"
#include "el/util/space_allocator.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using el::util::SpaceAllocator;

EL_TEST_GROUP(tb_space_allocator) {
  EL_TEST(free_when_full) {
    SpaceAllocator spa(30);
    EL_VERIFY(spa.HasSpace(30));
    SpaceAllocator::Space* s1 = spa.AllocSpace(10);
    SpaceAllocator::Space* s2 = spa.AllocSpace(10);
    SpaceAllocator::Space* s3 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s2 && s3);
    EL_VERIFY(!spa.HasSpace(1));

    // Free middle
    spa.FreeSpace(s2);
    EL_VERIFY(spa.HasSpace(10));

    // Allocate middle (again)
    s2 = spa.AllocSpace(10);
    EL_VERIFY(!spa.HasSpace(10));

    // Free first
    spa.FreeSpace(s1);
    EL_VERIFY(spa.HasSpace(10));

    // Allocate first (again)
    s1 = spa.AllocSpace(10);
    EL_VERIFY(!spa.HasSpace(10));

    // Free last
    spa.FreeSpace(s3);
    EL_VERIFY(spa.HasSpace(10));

    // Allocate last (again)
    s3 = spa.AllocSpace(10);
    EL_VERIFY(!spa.HasSpace(10));
  }
  EL_TEST(free_all) {
    SpaceAllocator spa(30);
    {
      SpaceAllocator::Space* s = spa.AllocSpace(30);
      spa.FreeSpace(s);
      EL_VERIFY(spa.HasSpace(30));
    }

    // Free in order
    SpaceAllocator::Space* s1 = spa.AllocSpace(10);
    SpaceAllocator::Space* s2 = spa.AllocSpace(10);
    SpaceAllocator::Space* s3 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s2 && s3);
    spa.FreeSpace(s1);
    spa.FreeSpace(s2);
    spa.FreeSpace(s3);
    EL_VERIFY(spa.HasSpace(30));

    // Free in backwards order
    s1 = spa.AllocSpace(10);
    s2 = spa.AllocSpace(10);
    s3 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s2 && s3);
    spa.FreeSpace(s3);
    spa.FreeSpace(s2);
    spa.FreeSpace(s1);
    EL_VERIFY(spa.HasSpace(30));

    // Free middle first (in order)
    s1 = spa.AllocSpace(10);
    s2 = spa.AllocSpace(10);
    s3 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s2 && s3);
    spa.FreeSpace(s2);
    spa.FreeSpace(s1);
    spa.FreeSpace(s3);
    EL_VERIFY(spa.HasSpace(30));

    // Free middle first (in backwards order)
    s1 = spa.AllocSpace(10);
    s2 = spa.AllocSpace(10);
    s3 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s2 && s3);
    spa.FreeSpace(s2);
    spa.FreeSpace(s3);
    spa.FreeSpace(s1);
    EL_VERIFY(spa.HasSpace(30));
  }
  EL_TEST(free_scattered) {
    SpaceAllocator spa(50);
    SpaceAllocator::Space* s1 = spa.AllocSpace(10);
    SpaceAllocator::Space* s2 = spa.AllocSpace(10);
    SpaceAllocator::Space* s3 = spa.AllocSpace(10);
    SpaceAllocator::Space* s4 = spa.AllocSpace(10);
    SpaceAllocator::Space* s5 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s2 && s3 && s4 && s5);

    // Free all middle space
    spa.FreeSpace(s2);
    spa.FreeSpace(s4);
    spa.FreeSpace(s3);
    EL_VERIFY(spa.HasSpace(30));

    s2 = spa.AllocSpace(10);
    s3 = spa.AllocSpace(10);
    s4 = spa.AllocSpace(10);

    // Free edges, then middle
    spa.FreeSpace(s1);
    spa.FreeSpace(s5);
    spa.FreeSpace(s3);
    EL_VERIFY(!spa.HasSpace(30));  // We should not have 30 of continous space.
    s1 = spa.AllocSpace(10);
    s3 = spa.AllocSpace(10);
    s5 = spa.AllocSpace(10);
    EL_VERIFY(s1 && s3 && s5);  // We should have 3 * 10 spaces though.
  }
}

#endif  // EL_UNIT_TESTING
