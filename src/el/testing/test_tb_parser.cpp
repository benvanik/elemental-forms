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

EL_TEST_GROUP(tb_parser) {
  ParseNode node;
  EL_TEST(Init) {
    EL_VERIFY(node.ReadFile(EL_TEST_FILE("data/test_tb_parser.tb.txt")));
  }

  EL_TEST(strings) {
    EL_VERIFY_STR(node.GetValueString("strings>string1", ""), "A string");
    EL_VERIFY_STR(node.GetValueString("strings>string2", ""), "\"A string\"");
    EL_VERIFY_STR(node.GetValueString("strings>string3", ""), "\'A string\'");
    EL_VERIFY_STR(node.GetValueString("strings>string4", ""),
                  "\"\'A string\'\"");
    EL_VERIFY_STR(node.GetValueString("strings>string5", ""), "Foo\nBar");
  }

  EL_TEST(strings_compact) {
    EL_VERIFY_STR(node.GetValueString("strings_compact>string1", ""), "");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string2", ""),
                  "A string");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string3", ""),
                  "A string");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string4", ""),
                  "'A string'");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string5", ""),
                  "\"A string\"");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string6", ""),
                  "\"A string\"");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string7", ""), "\\");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string8", ""), "\"");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string9", ""),
                  "\\\\\\\\");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string10", ""),
                  "\\\\\"");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string11", ""),
                  "\"\"\'\'");
    EL_VERIFY_STR(node.GetValueStringRaw("strings_compact>string12", ""),
                  "@language_string_token");
    EL_VERIFY_STR(node.GetValueString("strings_compact>string13", ""),
                  "#ffdd00");
  }

  EL_TEST(numbers) {
    EL_VERIFY(node.GetValueInt("numbers>integer1", 0) == 42);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers>float1", 0), 1.1);
    EL_VERIFY(node.GetValueInt("numbers>integer2", 0) == 42);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers>float2", 0), 1.1);
  }

  EL_TEST(numbers_compact) {
    EL_VERIFY(node.GetValueInt("numbers_compact>integer1", 0) == -10);
    EL_VERIFY(node.GetValueInt("numbers_compact>integer2", 0) == -10);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_compact>float1", 0), 1.0);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_compact>float2", 0), -1.0);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_compact>float3", 0), -.2);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_compact>float4", 0), -.2);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_compact>float5", 0), 1.1);
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_compact>float6", 0), 1.1);
  }

  EL_TEST(numbers_with_unit) {
    EL_VERIFY(node.GetValueInt("numbers_with_unit>number1", 0) == 10);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number1", ""), "10px");
    EL_VERIFY(node.GetValueInt("numbers_with_unit>number2", 0) == -10);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number2", ""),
                  "-10px");
    EL_VERIFY(node.GetValueInt("numbers_with_unit>number3", 0) == 10);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number3", ""), "10px");
    EL_VERIFY(node.GetValueInt("numbers_with_unit>number4", 0) == -10);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number4", ""),
                  "-10px");
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_with_unit>number5", 0), -2.1);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number5", ""),
                  "-2.1px");
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_with_unit>number6", 0), -.1);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number6", ""),
                  "-.1px");
    EL_VERIFY_FLOAT(node.GetValueFloat("numbers_with_unit>number7", 0), 1.f);
    EL_VERIFY_STR(node.GetValueString("numbers_with_unit>number7", ""), "1px ");
  }

  EL_TEST(compact_with_children) {
    EL_VERIFY_STR(node.GetValueString("compact_with_children>string", ""),
                  "A string");
    EL_VERIFY_STR(
        node.GetValueString("compact_with_children>string>child1", ""),
        "Child 1");
    EL_VERIFY_STR(
        node.GetValueString("compact_with_children>string>child2", ""),
        "Child 2");

    EL_VERIFY(node.GetValueInt("compact_with_children>integer", 0) == -10);
    EL_VERIFY(node.GetValueInt("compact_with_children>integer>child1", 0) == 1);
    EL_VERIFY(node.GetValueInt("compact_with_children>integer>child2", 0) == 2);

    EL_VERIFY_FLOAT(node.GetValueFloat("compact_with_children>float", 0), 1);
    EL_VERIFY_FLOAT(node.GetValueFloat("compact_with_children>float>child1", 0),
                    1);
    EL_VERIFY_FLOAT(node.GetValueFloat("compact_with_children>float>child2", 0),
                    2);
  }

  EL_TEST(compact_no_value) {
    EL_VERIFY_STR(node.GetValueString("compact_no_value>string", ""),
                  "A string");
    EL_VERIFY(node.GetValueInt("compact_no_value>int", 0) == 42);
    EL_VERIFY_FLOAT(node.GetValueFloat("compact_no_value>float", 0), 3.14);
    EL_VERIFY_STR(node.GetValueString("compact_no_value>subgroup>string1", ""),
                  "A string, with \"comma\"");
    EL_VERIFY_STR(node.GetValueString("compact_no_value>subgroup>string2", ""),
                  "'Another string'");
    EL_VERIFY_STR(node.GetValueString("compact_no_value>subgroup>string3", ""),
                  "And another string");
  }

  EL_TEST(arrays_numbers) {
    ParseNode* arr_n = node.GetNode("arrays>numbers");
    EL_VERIFY(arr_n);
    EL_VERIFY(arr_n->value().array_size() == 5);
    ValueArray* arr = arr_n->value().as_array();
    EL_VERIFY(arr->at(0)->as_integer() == 1);
    EL_VERIFY(arr->at(1)->as_integer() == 2);
    EL_VERIFY_FLOAT(arr->at(2)->as_float(), 0.5);
    EL_VERIFY_FLOAT(arr->at(3)->as_float(), 1.0E-8);
    EL_VERIFY(arr->at(4)->as_integer() == 1000000000);
  }

  EL_TEST(arrays_dimensions) {
    ParseNode* arr_n = node.GetNode("arrays>dimensions");
    EL_VERIFY(arr_n);
    EL_VERIFY(arr_n->value().array_size() == 2);
    ValueArray* arr = arr_n->value().as_array();
    EL_VERIFY(arr->at(0)->as_integer() == 1);
    EL_VERIFY(arr->at(1)->as_integer() == 2);
  }

  // FIX: Not supported yet
  //	TB_TEST(arrays_strings)
  //	{
  //		Node *arr_n = node.GetNode("arrays>strings");
  //		TB_VERIFY(arr_n);
  //		TB_VERIFY(arr_n->value().array_size() == 5);
  //		ValueArray *arr = arr_n->value().as_array();
  //		TB_VERIFY_STR(arr->GetValue(0)->as_string(), "Foo");
  //		TB_VERIFY_STR(arr->GetValue(1)->as_string(), "'Foo'");
  //		TB_VERIFY_STR(arr->GetValue(2)->as_string(), "Foo");
  //		TB_VERIFY_STR(arr->GetValue(3)->as_string(), "\"Foo\"");
  //		TB_VERIFY_STR(arr->GetValue(4)->as_string(), "Foo 'bar'");
  //	}
  //
  //	TB_TEST(arrays_mixed)
  //	{
  //		Node *arr_n = node.GetNode("arrays>mixed");
  //		TB_VERIFY(arr_n);
  //		TB_VERIFY(arr_n->value().array_size() == 4);
  //		ValueArray *arr = arr_n->value().as_array();
  //		TB_VERIFY_STR(arr->GetValue(0)->as_string(), "Foo");
  //		TB_VERIFY(arr->GetValue(1)->as_integer() == 2);
  //		TB_VERIFY_STR(arr->GetValue(2)->as_string(), "bar");
  //		TB_VERIFY(arr->GetValue(3)->as_float() == 4.0f);
  //	}

  EL_TEST(strings_multiline) {
    EL_VERIFY_STR(node.GetValueString("strings_multiline>string1", ""),
                  "Line 1\nLine 2\nLine 3");
    EL_VERIFY_STR(node.GetValueString("strings_multiline>string2", ""), "abc");
    EL_VERIFY_STR(node.GetValueString("strings_multiline>string3", ""), "AB");
    EL_VERIFY_STR(node.GetValueString("strings_multiline>string4", ""),
                  "Line 1\nLine 2\nLine 3\n");
    EL_VERIFY_STR(node.GetValueString("strings_multiline>subgroup>first", ""),
                  "Foo");
    EL_VERIFY_STR(node.GetValueString("strings_multiline>subgroup>second", ""),
                  "AB");
    EL_VERIFY_STR(node.GetValueString("strings_multiline>string5", ""),
                  "The last string");
  }

  EL_TEST(comments_and_space) {
    EL_VERIFY(node.GetValueInt("comments_and_space>one", 0) == 1);
    EL_VERIFY(node.GetValueInt("comments_and_space>two", 0) == 2);
    EL_VERIFY(node.GetValueInt("comments_and_space>three", 0) == 3);
  }

  EL_TEST(include_file) {
    EL_VERIFY_STR(node.GetValueString("include_file>file1>something1", ""),
                  "Chocolate");
    EL_VERIFY_STR(node.GetValueString("include_file>file1>something2", ""),
                  "Cake");
    EL_VERIFY_STR(node.GetValueString("include_file>file2>something1", ""),
                  "Chocolate");
    EL_VERIFY_STR(node.GetValueString("include_file>file2>something2", ""),
                  "Cake");
  }

  EL_TEST(include_locally) {
    EL_VERIFY_STR(node.GetValueString("include_branch>test1>skin", ""),
                  "DarkSkin");
    EL_VERIFY_STR(node.GetValueString("include_branch>test2>skin", ""),
                  "LightSkin");
  }

  EL_TEST(conditions) {
    EL_VERIFY(node.GetValueInt("conditions>value", 0) == 42);
  }

  EL_TEST(local_ref) {
    EL_VERIFY_STR(node.GetValueString("defines_test>test1", ""), "#ffdd00");
    EL_VERIFY_STR(node.GetValueString("defines_test>test2", ""), "#ffdd00");
    EL_VERIFY_STR(node.GetValueString("defines_test>broken", ""),
                  "@>defines>colors>broken");
    EL_VERIFY_STR(node.GetValueString("defines_test>cycle", ""),
                  "@>defines_test>cycle");
  }

  // More coverage in test_tb_node_ref_tree.cpp...
}

#endif  // EL_UNIT_TESTING
