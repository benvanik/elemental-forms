/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"
#include "tb/color.h"

#ifdef TB_UNIT_TESTING

using namespace tb;

TB_TEST_GROUP(tb_color) {
  TB_TEST(set_from_string_rrggbbaa) {
    Color col;
    col.reset("#11223344", 9);
    TB_VERIFY(col.r == 0x11);
    TB_VERIFY(col.g == 0x22);
    TB_VERIFY(col.b == 0x33);
    TB_VERIFY(col.a == 0x44);
  }
  TB_TEST(set_from_string_rrggbb) {
    Color col;
    col.reset("#112233", 7);
    TB_VERIFY(col.r == 0x11);
    TB_VERIFY(col.g == 0x22);
    TB_VERIFY(col.b == 0x33);
    TB_VERIFY(col.a == 0xff);
  }
  TB_TEST(set_from_string_rgba) {
    Color col;
    col.reset("#1234", 5);
    TB_VERIFY(col.r == 0x11);
    TB_VERIFY(col.g == 0x22);
    TB_VERIFY(col.b == 0x33);
    TB_VERIFY(col.a == 0x44);
  }
  TB_TEST(set_from_string_rgb) {
    Color col;
    col.reset("#123", 4);
    TB_VERIFY(col.r == 0x11);
    TB_VERIFY(col.g == 0x22);
    TB_VERIFY(col.b == 0x33);
    TB_VERIFY(col.a == 0xff);
  }
  TB_TEST(set_from_string_invalid) {
    Color col;
    col.reset("123", 3);
    TB_VERIFY(col.r == 0x0);
    TB_VERIFY(col.g == 0x0);
    TB_VERIFY(col.b == 0x0);
    TB_VERIFY(col.a == 0xff);
  }
}

#endif  // TB_UNIT_TESTING
