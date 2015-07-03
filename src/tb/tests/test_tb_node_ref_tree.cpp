/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_test.h"
#include "tb_node_ref_tree.h"
#include "tb_widgets_reader.h"
#include "tb_layout.h"
#include "tb_text_box.h"

#ifdef TB_UNIT_TESTING

using namespace tb;

TB_TEST_GROUP(tb_node_ref_tree) {
  class DataListener : public NodeRefTreeListener {
   public:
    std::string changed_request;
    int changed_counter;
    DataListener() : changed_counter(0) {}
    virtual void OnDataChanged(NodeRefTree* dt, const char* request) {
      changed_request.Set(request);
      changed_counter++;
    }
  };

  TB_TEST(change_on_set) {
    NodeRefTree dt("r");
    DataListener dl;
    dt.AddListener(&dl);

    dt.SetValue("mainbar>size", Value("10mm", Value::Set::kAsStatic));
    TB_VERIFY_STR(dl.changed_request, "mainbar>size");

    dt.SetValue("mainbar>axis", Value("x", Value::Set::kAsStatic));
    TB_VERIFY_STR(dl.changed_request, "mainbar>axis");

    dt.SetValue("mainbar>visible", Value(42));
    TB_VERIFY_STR(dl.changed_request, "mainbar>visible");

    TB_VERIFY_STR(dt.GetValue("mainbar>size").GetString(), "10mm");
    TB_VERIFY_STR(dt.GetValue("mainbar>axis").GetString(), "x");
    TB_VERIFY(dt.GetValue("mainbar>visible").GetInt() == 42);

    TB_VERIFY(dl.changed_counter == 3);

    dt.RemoveListener(&dl);
  }

  TB_TEST(reference_value) {
    NodeRefTree dt("test_styles");
    dt.ReadData(
        "FireButton\n"
        "	skin: 'FireButtonSkin'\n");

    Widget root;
    g_widgets_reader->LoadData(
        &root, "Button: id: 'fire', skin: '@test_styles>FireButton>skin'");

    Widget* button = root.GetWidgetByID(TBIDC("fire"));
    TB_VERIFY(button->GetSkinBg() == TBIDC("FireButtonSkin"));
  }

  TB_TEST(reference_value_recurse) {
    NodeRefTree dt1("test_foo");
    dt1.ReadData(
        "foo_value: 42\n"
        "foo_circular: '@test_bar>bar_circular'\n");
    NodeRefTree dt2("test_bar");
    dt2.ReadData(
        "bar_value: '@test_foo>foo_value'\n"
        "bar_circular: '@test_foo>foo_circular'\n"
        "bar_circular2: '@test_bar>bar_circular'\n"
        "bar_broken_node: '@test_foo>foo_broken_node'"
        "bar_broken_tree: '@test_foo>foo_broken_tree'");

    Widget root;
    g_widgets_reader->LoadData(
        &root,
        "SelectInline: id: 'select', value: '@test_bar>bar_value'\n"
        "Button: id: 'button_circular', text: '@test_bar>bar_circular'\n"
        "Button: id: 'button_broken_node', text: "
        "'@test_bar>bar_broken_node'\n"
        "Button: id: 'button_broken_tree', text: '@test_bad_tree>foo'\n");

    // Reference from resource to one tree to another tree.
    Widget* select = root.GetWidgetByID(TBIDC("select"));
    TB_VERIFY(select->GetValue() == 42);

    // Reference in a circular loop. Should not freeze.
    Widget* button_circular = root.GetWidgetByID(TBIDC("button_circular"));
    TB_VERIFY_STR(button_circular->GetText(), "@test_bar>bar_circular");

    // Reference in a circular loop. Should not freeze.
    TB_VERIFY(NodeRefTree::GetValueFromTree("@test_bar>bar_circular2")
                  .GetType() == Value::Type::kNull);

    // References tree is wrong
    TB_VERIFY(NodeRefTree::GetValueFromTree("@test_bad_tree>does_not_exist")
                  .GetType() == Value::Type::kNull);

    // Reference that is broken (has no matching node).
    Widget* button_broken1 = root.GetWidgetByID(TBIDC("button_broken_node"));
    TB_VERIFY_STR(button_broken1->GetText(), "@test_foo>foo_broken_node");

    // Reference that is broken (has no matching tree).
    Widget* button_broken2 = root.GetWidgetByID(TBIDC("button_broken_tree"));
    TB_VERIFY_STR(button_broken2->GetText(), "@test_bad_tree>foo");
  }

  TB_TEST(reference_include) {
    NodeRefTree dt("test_styles");
    dt.ReadData(
        "VeryNice\n"
        "	skin: 'SpecialSkin'\n"
        "	text: 'hello'\n");

    Widget root;
    g_widgets_reader->LoadData(&root,
                               "TextBox: id: 'edit'\n"
                               "	@include @test_styles>VeryNice");
    TextBox* edit = root.GetWidgetByIDAndType<TextBox>(TBIDC("edit"));
    TB_VERIFY(edit->GetSkinBg() == TBIDC("SpecialSkin"));
    TB_VERIFY_STR(edit->GetText(), "hello");
  }

  TB_TEST(reference_local_include) {
    Widget root;
    g_widgets_reader->LoadData(&root,
                               "SomeDeclarations\n"
                               "	skin: 'SpecialSkin'\n"
                               "	text: 'hello'\n"
                               "TextBox: id: 'edit'\n"
                               "	@include SomeDeclarations");
    TextBox* edit = root.GetWidgetByIDAndType<TextBox>(TBIDC("edit"));
    TB_VERIFY(edit->GetSkinBg() == TBIDC("SpecialSkin"));
    TB_VERIFY_STR(edit->GetText(), "hello");
  }

  TB_TEST(reference_condition) {
    NodeRefTree dt("test_settings");
    dt.ReadData(
        "layout\n"
        "	landscape: 1\n");

    const char* layout_str =
        "Layout: id: 'layout'\n"
        "	distribution: 'available'\n"
        "	@if @test_settings>layout>landscape\n"
        "		axis: 'x'\n"
        "		spacing: 100px\n"
        "	@else\n"
        "		axis: 'y'\n"
        "		spacing: 200px\n"
        "	gravity: 'all'\n";

    Widget root1, root2;

    // Inflate & check
    g_widgets_reader->LoadData(&root1, layout_str);
    Layout* layout1 = root1.GetWidgetByIDAndType<Layout>(TBIDC("layout"));
    TB_VERIFY(layout1->GetAxis() == Axis::kX);
    TB_VERIFY(layout1->GetSpacing() == 100);
    TB_VERIFY(layout1->GetGravity() == Gravity::kAll);

    // Change data for condition
    dt.SetValue("layout>landscape", Value(0));

    // Inflate & check
    g_widgets_reader->LoadData(&root2, layout_str);
    Layout* layout2 = root2.GetWidgetByIDAndType<Layout>(TBIDC("layout"));
    TB_VERIFY(layout2->GetAxis() == Axis::kY);
    TB_VERIFY(layout2->GetSpacing() == 200);
    TB_VERIFY(layout2->GetGravity() == Gravity::kAll);
  }

  Node* GetChildNodeFromIndex(Node * parent, int index) {
    Node* child = parent->GetFirstChild();
    while (child && index-- > 0) child = child->GetNext();
    return child;
  }

  TB_TEST(reference_condition_branch_insert_order) {
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
    Node node;
    node.ReadData(str);

    Node* a = GetChildNodeFromIndex(&node, 0);
    TB_VERIFY_STR(a->GetName(), "A");
    TB_VERIFY_STR(GetChildNodeFromIndex(a, 0)->GetName(), "B1");
    TB_VERIFY_STR(
        GetChildNodeFromIndex(GetChildNodeFromIndex(a, 0), 0)->GetName(), "C1");
    TB_VERIFY_STR(GetChildNodeFromIndex(a, 1)->GetName(), "B2");
    TB_VERIFY_STR(GetChildNodeFromIndex(a, 2)->GetName(), "B3");
    TB_VERIFY_STR(GetChildNodeFromIndex(a, 3)->GetName(), "B4");
    TB_VERIFY_STR(GetChildNodeFromIndex(a, 4)->GetName(), "B5");
  }
}

#endif  // TB_UNIT_TESTING
