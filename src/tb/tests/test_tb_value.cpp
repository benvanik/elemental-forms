/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"
#include "tb/parsing/parse_node.h"

#ifdef TB_UNIT_TESTING

using namespace tb;
using namespace tb::parsing;

TB_TEST_GROUP(tb_value) {
  TB_TEST(node_create_on_get) {
    ParseNode node;
    TB_VERIFY(node.GetNode("foo>bar>funky") == nullptr);
    TB_VERIFY(node.GetNode("foo>bar>funky",
                           ParseNode::MissingPolicy::kCreate) != nullptr);
    TB_VERIFY(node.GetNode("foo>bar>funky") != nullptr);
  }

  // More coverage in test_tb_parser.cpp...
}

#endif  // TB_UNIT_TESTING
