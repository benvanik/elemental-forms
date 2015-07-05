/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"

#include "tb/util/string_builder.h"

#ifdef TB_UNIT_TESTING

using namespace tb;
using namespace tb::util;

TB_TEST_GROUP(tb_string_builder) {
  TB_TEST(append_path_1) {
    StringBuilder buf;
    buf.AppendPath("foo.txt");
    TB_VERIFY_STR(buf.data(), "./");
  }
  TB_TEST(append_path_2) {
    StringBuilder buf;
    buf.AppendPath("Path/subpath/foo.txt");
    TB_VERIFY_STR(buf.data(), "Path/subpath/");
  }
  TB_TEST(append_path_3) {
    StringBuilder buf;
    buf.AppendPath("C:\\test\\foo.txt");
    TB_VERIFY_STR(buf.data(), "C:\\test\\");
  }
  TB_TEST(append_string) {
    StringBuilder buf;
    buf.AppendString("xxxxxxxxxx");
    TB_VERIFY(buf.GetAppendPos() == 10);
    TB_VERIFY_STR(buf.data(), "xxxxxxxxxx");

    buf.SetAppendPos(0);
    buf.AppendString("Foo");
    buf.AppendString("Bar");
    TB_VERIFY_STR(buf.data(), "FooBar");
  }
}

#endif  // TB_UNIT_TESTING
