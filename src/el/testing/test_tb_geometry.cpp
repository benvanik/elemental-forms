/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/rect.h"
#include "el/testing/testing.h"
#include "el/util/rect_region.h"

#ifdef EL_UNIT_TESTING

using namespace el;

EL_TEST_GROUP(tb_geometry) {
  EL_TEST(RectRegion_include) {
    util::RectRegion region;

    EL_VERIFY(region.IncludeRect(Rect(10, 10, 100, 100)));
    EL_VERIFY(region.IncludeRect(Rect(50, 50, 100, 100)));
    EL_VERIFY(region.size() == 3);
    EL_VERIFY(region[0].equals(Rect(10, 10, 100, 100)));
    EL_VERIFY(region[1].equals(Rect(110, 50, 40, 60)));
    EL_VERIFY(region[2].equals(Rect(50, 110, 100, 40)));
  }

  EL_TEST(RectRegion_include_adjecent_coalesce) {
    util::RectRegion region;

    EL_VERIFY(region.IncludeRect(Rect(10, 10, 10, 10)));

    // extend right
    EL_VERIFY(region.IncludeRect(Rect(20, 10, 10, 10)));
    EL_VERIFY(region.size() == 1);
    EL_VERIFY(region[0].equals(Rect(10, 10, 20, 10)));

    // extend bottom
    EL_VERIFY(region.IncludeRect(Rect(10, 20, 20, 10)));
    EL_VERIFY(region.size() == 1);
    EL_VERIFY(region[0].equals(Rect(10, 10, 20, 20)));

    // extend left
    EL_VERIFY(region.IncludeRect(Rect(0, 10, 10, 20)));
    EL_VERIFY(region.size() == 1);
    EL_VERIFY(region[0].equals(Rect(0, 10, 30, 20)));

    // extend top
    EL_VERIFY(region.IncludeRect(Rect(0, 0, 30, 10)));
    EL_VERIFY(region.size() == 1);
    EL_VERIFY(region[0].equals(Rect(0, 0, 30, 30)));
  }

  EL_TEST(RectRegion_include_overlap_coalesce) {
    util::RectRegion region;

    EL_VERIFY(region.IncludeRect(Rect(10, 10, 10, 10)));

    // extend to left and right
    EL_VERIFY(region.IncludeRect(Rect(0, 10, 30, 10)));
    EL_VERIFY(region.size() == 1);
    EL_VERIFY(region[0].equals(Rect(0, 10, 30, 10)));
  }

  EL_TEST(RectRegion_include_overlap_multi_coalesce) {
    util::RectRegion region;

    EL_VERIFY(region.IncludeRect(Rect(10, 10, 10, 10)));

    // extend in all directions at once
    EL_VERIFY(region.IncludeRect(Rect(0, 0, 100, 100)));
    EL_VERIFY(region.size() == 1);
    EL_VERIFY(region[0].equals(Rect(0, 0, 100, 100)));
  }

  EL_TEST(RectRegion_exclude) {
    util::RectRegion region;

    EL_VERIFY(region.Set(Rect(100, 100, 100, 100)));

    // Exclude in the middle (cut a 20x20 hole)
    EL_VERIFY(region.ExcludeRect(Rect(140, 140, 20, 20)));
    EL_VERIFY(region.size() == 4);
    EL_VERIFY(region[0].equals(Rect(100, 100, 100, 40)));
    EL_VERIFY(region[1].equals(Rect(100, 140, 40, 20)));
    EL_VERIFY(region[2].equals(Rect(160, 140, 40, 20)));
    EL_VERIFY(region[3].equals(Rect(100, 160, 100, 40)));

    // Exclude in the middle (cut a 40x40 hole)
    EL_VERIFY(region.ExcludeRect(Rect(130, 130, 40, 40)));
    EL_VERIFY(region.size() == 4);
    EL_VERIFY(region[0].equals(Rect(100, 100, 100, 30)));
    EL_VERIFY(region[1].equals(Rect(100, 130, 30, 40)));
    EL_VERIFY(region[2].equals(Rect(170, 130, 30, 40)));
    EL_VERIFY(region[3].equals(Rect(100, 170, 100, 30)));
  }
}

#endif  // EL_UNIT_TESTING
