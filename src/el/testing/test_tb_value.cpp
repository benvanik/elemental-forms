/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/parsing/parse_node.h"
#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using namespace el::parsing;

EL_TEST_GROUP(tb_value) {
  EL_TEST(node_create_on_get) {
    ParseNode node;
    EL_VERIFY(node.GetNode("foo>bar>funky") == nullptr);
    EL_VERIFY(node.GetNode("foo>bar>funky",
                           ParseNode::MissingPolicy::kCreate) != nullptr);
    EL_VERIFY(node.GetNode("foo>bar>funky") != nullptr);
  }

  // More coverage in test_tb_parser.cpp...
}

#endif  // EL_UNIT_TESTING
