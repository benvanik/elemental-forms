/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/layout_box.h"
#include "el/elements/text_box.h"
#include "el/parsing/parse_node.h"
#include "el/parsing/parse_node_tree.h"
#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using namespace el::elements;
using namespace el::parsing;

EL_TEST_GROUP(tb_node_ref_tree) {
  class DataListener : public ParseNodeTreeListener {
   public:
    std::string changed_request;
    int changed_counter;
    DataListener() : changed_counter(0) {}
    virtual void OnDataChanged(ParseNodeTree* dt, const char* request) {
      changed_request = request;
      changed_counter++;
    }
  };

  EL_TEST(change_on_set) {
    ParseNodeTree dt("r");
    DataListener dl;
    dt.AddListener(&dl);

    dt.SetValue("mainbar>size", Value("10mm", Value::Set::kAsStatic));
    EL_VERIFY_STR(dl.changed_request, "mainbar>size");

    dt.SetValue("mainbar>axis", Value("x", Value::Set::kAsStatic));
    EL_VERIFY_STR(dl.changed_request, "mainbar>axis");

    dt.SetValue("mainbar>visible", Value(42));
    EL_VERIFY_STR(dl.changed_request, "mainbar>visible");

    EL_VERIFY_STR(dt.GetValue("mainbar>size").as_string(), "10mm");
    EL_VERIFY_STR(dt.GetValue("mainbar>axis").as_string(), "x");
    EL_VERIFY(dt.GetValue("mainbar>visible").as_integer() == 42);

    EL_VERIFY(dl.changed_counter == 3);

    dt.RemoveListener(&dl);
  }

  EL_TEST(reference_value) {
    ParseNodeTree dt("test_styles");
    dt.ReadData(
        "FireButton\n"
        "	skin: 'FireButtonSkin'\n");

    Element root;
    root.LoadData("Button: id: 'fire', skin: '@test_styles>FireButton>skin'");

    auto button = root.GetElementById<Element>(TBIDC("fire"));
    EL_VERIFY(button->background_skin() == TBIDC("FireButtonSkin"));
  }

  EL_TEST(reference_value_recurse) {
    ParseNodeTree dt1("test_foo");
    dt1.ReadData(
        "foo_value: 42\n"
        "foo_circular: '@test_bar>bar_circular'\n");
    ParseNodeTree dt2("test_bar");
    dt2.ReadData(
        "bar_value: '@test_foo>foo_value'\n"
        "bar_circular: '@test_foo>foo_circular'\n"
        "bar_circular2: '@test_bar>bar_circular'\n"
        "bar_broken_node: '@test_foo>foo_broken_node'"
        "bar_broken_tree: '@test_foo>foo_broken_tree'");

    Element root;
    root.LoadData(
        "SpinBox: id: 'select', value: '@test_bar>bar_value'\n"
        "Button: id: 'button_circular', text: '@test_bar>bar_circular'\n"
        "Button: id: 'button_broken_node', text: "
        "'@test_bar>bar_broken_node'\n"
        "Button: id: 'button_broken_tree', text: '@test_bad_tree>foo'\n");

    // Reference from resource to one tree to another tree.
    auto select = root.GetElementById<Element>(TBIDC("select"));
    EL_VERIFY(select->value() == 42);

    // Reference in a circular loop. Should not freeze.
    auto button_circular =
        root.GetElementById<Element>(TBIDC("button_circular"));
    EL_VERIFY_STR(button_circular->text(), "@test_bar>bar_circular");

    // Reference in a circular loop. Should not freeze.
    EL_VERIFY(ParseNodeTree::GetValueFromTree("@test_bar>bar_circular2")
                  .type() == Value::Type::kNull);

    // References tree is wrong
    EL_VERIFY(ParseNodeTree::GetValueFromTree("@test_bad_tree>does_not_exist")
                  .type() == Value::Type::kNull);

    // Reference that is broken (has no matching node).
    auto button_broken1 =
        root.GetElementById<Element>(TBIDC("button_broken_node"));
    EL_VERIFY_STR(button_broken1->text(), "@test_foo>foo_broken_node");

    // Reference that is broken (has no matching tree).
    auto button_broken2 =
        root.GetElementById<Element>(TBIDC("button_broken_tree"));
    EL_VERIFY_STR(button_broken2->text(), "@test_bad_tree>foo");
  }

  EL_TEST(reference_include) {
    ParseNodeTree dt("test_styles");
    dt.ReadData(
        "VeryNice\n"
        "	skin: 'SpecialSkin'\n"
        "	text: 'hello'\n");

    Element root;
    root.LoadData(
        "TextBox: id: 'edit'\n"
        "	@include @test_styles>VeryNice");
    auto edit = root.GetElementById<TextBox>(TBIDC("edit"));
    EL_VERIFY(edit->background_skin() == TBIDC("SpecialSkin"));
    EL_VERIFY_STR(edit->text(), "hello");
  }

  EL_TEST(reference_local_include) {
    Element root;
    root.LoadData(
        "SomeDeclarations\n"
        "	skin: 'SpecialSkin'\n"
        "	text: 'hello'\n"
        "TextBox: id: 'edit'\n"
        "	@include SomeDeclarations");
    auto edit = root.GetElementById<TextBox>(TBIDC("edit"));
    EL_VERIFY(edit->background_skin() == TBIDC("SpecialSkin"));
    EL_VERIFY_STR(edit->text(), "hello");
  }

  EL_TEST(reference_condition) {
    ParseNodeTree dt("test_settings");
    dt.ReadData(
        "layout\n"
        "	landscape: 1\n");

    const char* layout_str =
        "LayoutBox: id: 'layout'\n"
        "	distribution: 'available'\n"
        "	@if @test_settings>layout>landscape\n"
        "		axis: 'x'\n"
        "		spacing: 100px\n"
        "	@else\n"
        "		axis: 'y'\n"
        "		spacing: 200px\n"
        "	gravity: 'all'\n";

    Element root1, root2;

    // Inflate & check
    root1.LoadData(layout_str);
    auto layout1 = root1.GetElementById<LayoutBox>(TBIDC("layout"));
    EL_VERIFY(layout1->axis() == Axis::kX);
    EL_VERIFY(layout1->spacing() == 100);
    EL_VERIFY(layout1->gravity() == Gravity::kAll);

    // Change data for condition
    dt.SetValue("layout>landscape", Value(0));

    // Inflate & check
    root2.LoadData(layout_str);
    auto layout2 = root2.GetElementById<LayoutBox>(TBIDC("layout"));
    EL_VERIFY(layout2->axis() == Axis::kY);
    EL_VERIFY(layout2->spacing() == 200);
    EL_VERIFY(layout2->gravity() == Gravity::kAll);
  }

  ParseNode* GetChildNodeFromIndex(ParseNode * parent, int index) {
    ParseNode* child = parent->first_child();
    while (child && index-- > 0) child = child->GetNext();
    return child;
  }

  EL_TEST(reference_condition_branch_insert_order) {
    const char* str =
        "A\n"
        "	B1\n"
        "		C1\n"
        "	@if 1\n"
        "		@if 1\n"
        "			B2\n"
        "		B3\n"
        "		B4\n"
        "	B5\n";
    ParseNode node;
    node.ReadData(str);

    ParseNode* a = GetChildNodeFromIndex(&node, 0);
    EL_VERIFY_STR(a->name(), "A");
    EL_VERIFY_STR(GetChildNodeFromIndex(a, 0)->name(), "B1");
    EL_VERIFY_STR(GetChildNodeFromIndex(GetChildNodeFromIndex(a, 0), 0)->name(),
                  "C1");
    EL_VERIFY_STR(GetChildNodeFromIndex(a, 1)->name(), "B2");
    EL_VERIFY_STR(GetChildNodeFromIndex(a, 2)->name(), "B3");
    EL_VERIFY_STR(GetChildNodeFromIndex(a, 3)->name(), "B4");
    EL_VERIFY_STR(GetChildNodeFromIndex(a, 4)->name(), "B5");
  }
}

#endif  // EL_UNIT_TESTING
