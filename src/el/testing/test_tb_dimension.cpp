/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/testing/testing.h"
#include "el/util/dimension_converter.h"

#ifdef EL_UNIT_TESTING

using namespace el;

EL_TEST_GROUP(tb_dimension_converter) {
  util::DimensionConverter dim_conv;

  EL_TEST(Init) { dim_conv.SetDPI(100, 200); }

  EL_TEST(set_from_string_unspecified) {
    int px = dim_conv.GetPxFromString("50", 0);
    EL_VERIFY(px == 50 * 2);
  }

  EL_TEST(set_from_string_px) {
    int px = dim_conv.GetPxFromString("50px", 0);
    EL_VERIFY(px == 50);
  }

  EL_TEST(set_from_string_dp) {
    int px = dim_conv.GetPxFromString("50dp", 0);
    EL_VERIFY(px == 50 * 2);
  }
}

#endif  // EL_UNIT_TESTING
