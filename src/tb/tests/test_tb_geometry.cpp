/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"

#include "tb/rect.h"
#include "tb/util/rect_region.h"

#ifdef TB_UNIT_TESTING

using namespace tb;

TB_TEST_GROUP(tb_geometry) {
  TB_TEST(RectRegion_include) {
    util::RectRegion region;

    TB_VERIFY(region.IncludeRect(Rect(10, 10, 100, 100)));
    TB_VERIFY(region.IncludeRect(Rect(50, 50, 100, 100)));
    TB_VERIFY(region.size() == 3);
    TB_VERIFY(region[0].equals(Rect(10, 10, 100, 100)));
    TB_VERIFY(region[1].equals(Rect(110, 50, 40, 60)));
    TB_VERIFY(region[2].equals(Rect(50, 110, 100, 40)));
  }

  TB_TEST(RectRegion_include_adjecent_coalesce) {
    util::RectRegion region;

    TB_VERIFY(region.IncludeRect(Rect(10, 10, 10, 10)));

    // extend right
    TB_VERIFY(region.IncludeRect(Rect(20, 10, 10, 10)));
    TB_VERIFY(region.size() == 1);
    TB_VERIFY(region[0].equals(Rect(10, 10, 20, 10)));

    // extend bottom
    TB_VERIFY(region.IncludeRect(Rect(10, 20, 20, 10)));
    TB_VERIFY(region.size() == 1);
    TB_VERIFY(region[0].equals(Rect(10, 10, 20, 20)));

    // extend left
    TB_VERIFY(region.IncludeRect(Rect(0, 10, 10, 20)));
    TB_VERIFY(region.size() == 1);
    TB_VERIFY(region[0].equals(Rect(0, 10, 30, 20)));

    // extend top
    TB_VERIFY(region.IncludeRect(Rect(0, 0, 30, 10)));
    TB_VERIFY(region.size() == 1);
    TB_VERIFY(region[0].equals(Rect(0, 0, 30, 30)));
  }

  TB_TEST(RectRegion_include_overlap_coalesce) {
    util::RectRegion region;

    TB_VERIFY(region.IncludeRect(Rect(10, 10, 10, 10)));

    // extend to left and right
    TB_VERIFY(region.IncludeRect(Rect(0, 10, 30, 10)));
    TB_VERIFY(region.size() == 1);
    TB_VERIFY(region[0].equals(Rect(0, 10, 30, 10)));
  }

  TB_TEST(RectRegion_include_overlap_multi_coalesce) {
    util::RectRegion region;

    TB_VERIFY(region.IncludeRect(Rect(10, 10, 10, 10)));

    // extend in all directions at once
    TB_VERIFY(region.IncludeRect(Rect(0, 0, 100, 100)));
    TB_VERIFY(region.size() == 1);
    TB_VERIFY(region[0].equals(Rect(0, 0, 100, 100)));
  }

  TB_TEST(RectRegion_exclude) {
    util::RectRegion region;

    TB_VERIFY(region.Set(Rect(100, 100, 100, 100)));

    // Exclude in the middle (cut a 20x20 hole)
    TB_VERIFY(region.ExcludeRect(Rect(140, 140, 20, 20)));
    TB_VERIFY(region.size() == 4);
    TB_VERIFY(region[0].equals(Rect(100, 100, 100, 40)));
    TB_VERIFY(region[1].equals(Rect(100, 140, 40, 20)));
    TB_VERIFY(region[2].equals(Rect(160, 140, 40, 20)));
    TB_VERIFY(region[3].equals(Rect(100, 160, 100, 40)));

    // Exclude in the middle (cut a 40x40 hole)
    TB_VERIFY(region.ExcludeRect(Rect(130, 130, 40, 40)));
    TB_VERIFY(region.size() == 4);
    TB_VERIFY(region[0].equals(Rect(100, 100, 100, 30)));
    TB_VERIFY(region[1].equals(Rect(100, 130, 30, 40)));
    TB_VERIFY(region[2].equals(Rect(170, 130, 30, 40)));
    TB_VERIFY(region[3].equals(Rect(100, 170, 100, 30)));
  }
}

#endif  // TB_UNIT_TESTING
