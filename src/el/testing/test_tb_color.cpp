/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/color.h"
#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;

EL_TEST_GROUP(tb_color) {
  EL_TEST(set_from_string_rrggbbaa) {
    Color col;
    col.reset("#11223344", 9);
    EL_VERIFY(col.r == 0x11);
    EL_VERIFY(col.g == 0x22);
    EL_VERIFY(col.b == 0x33);
    EL_VERIFY(col.a == 0x44);
  }
  EL_TEST(set_from_string_rrggbb) {
    Color col;
    col.reset("#112233", 7);
    EL_VERIFY(col.r == 0x11);
    EL_VERIFY(col.g == 0x22);
    EL_VERIFY(col.b == 0x33);
    EL_VERIFY(col.a == 0xff);
  }
  EL_TEST(set_from_string_rgba) {
    Color col;
    col.reset("#1234", 5);
    EL_VERIFY(col.r == 0x11);
    EL_VERIFY(col.g == 0x22);
    EL_VERIFY(col.b == 0x33);
    EL_VERIFY(col.a == 0x44);
  }
  EL_TEST(set_from_string_rgb) {
    Color col;
    col.reset("#123", 4);
    EL_VERIFY(col.r == 0x11);
    EL_VERIFY(col.g == 0x22);
    EL_VERIFY(col.b == 0x33);
    EL_VERIFY(col.a == 0xff);
  }
  EL_TEST(set_from_string_invalid) {
    Color col;
    col.reset("123", 3);
    EL_VERIFY(col.r == 0x0);
    EL_VERIFY(col.g == 0x0);
    EL_VERIFY(col.b == 0x0);
    EL_VERIFY(col.a == 0xff);
  }
}

#endif  // EL_UNIT_TESTING
