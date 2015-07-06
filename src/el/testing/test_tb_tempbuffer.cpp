/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/testing/testing.h"
#include "el/util/string_builder.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using namespace el::util;

EL_TEST_GROUP(tb_string_builder) {
  EL_TEST(append_path_1) {
    StringBuilder buf;
    buf.AppendPath("foo.txt");
    EL_VERIFY_STR(buf.data(), "./");
  }
  EL_TEST(append_path_2) {
    StringBuilder buf;
    buf.AppendPath("Path/subpath/foo.txt");
    EL_VERIFY_STR(buf.data(), "Path/subpath/");
  }
  EL_TEST(append_path_3) {
    StringBuilder buf;
    buf.AppendPath("C:\\test\\foo.txt");
    EL_VERIFY_STR(buf.data(), "C:\\test\\");
  }
  EL_TEST(append_string) {
    StringBuilder buf;
    buf.AppendString("xxxxxxxxxx");
    EL_VERIFY(buf.GetAppendPos() == 10);
    EL_VERIFY_STR(buf.data(), "xxxxxxxxxx");

    buf.SetAppendPos(0);
    buf.AppendString("Foo");
    buf.AppendString("Bar");
    EL_VERIFY_STR(buf.data(), "FooBar");
  }
}

#endif  // EL_UNIT_TESTING
