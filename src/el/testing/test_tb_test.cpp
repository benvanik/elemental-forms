/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;

EL_TEST_GROUP(tb_test) {
  EL_TEST(single_test) { EL_PASS(); }
}

EL_TEST_GROUP(tb_test_multiple_calls) {
  int setup_calls;
  int test_calls;

  EL_TEST(Setup) { setup_calls++; }
  EL_TEST(test_1) {
    EL_VERIFY(setup_calls == 1);
    test_calls++;
  }
  EL_TEST(test_2) {
    EL_VERIFY(setup_calls == 2);
    test_calls++;
  }
  EL_TEST(Cleanup) { EL_VERIFY(test_calls > 0); }
}

#endif  // EL_UNIT_TESTING
