/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"
#include "elements/tb_text_box.h"
#include "elements/tb_widgets_common.h"
#include "elements/tb_select.h"
#include "elements/tb_inline_select.h"
#include "tb/element_value.h"
#include "tb/elements/check_box.h"
#include "tb/elements/slider.h"

#ifdef TB_UNIT_TESTING

using namespace tb;
using namespace tb::elements;

TB_TEST_GROUP(tb_widget_value_text) {
  ElementValue element_val(TBIDC("test value text"));
  Element* a, *b, *c;

  TB_TEST(Init) {
    TB_VERIFY(a = new TextBox);
    TB_VERIFY(b = new TextBox);
    TB_VERIFY(c = new TextBox);
  }

  TB_TEST(connect) {
    // Set the initial value, no elements connected yet.
    element_val.SetText("turbo badger");

    // Connecting elements should give them the current value.
    a->Connect(&element_val);
    b->Connect(&element_val);
    c->Connect(&element_val);

    TB_VERIFY_STR(a->GetText(), "turbo badger");
    TB_VERIFY_STR(b->GetText(), "turbo badger");
    TB_VERIFY_STR(c->GetText(), "turbo badger");
  }

  TB_TEST(change_value) {
    // Changing the value should change all elements
    element_val.SetText("Emil");

    TB_VERIFY_STR(a->GetText(), "Emil");
    TB_VERIFY_STR(b->GetText(), "Emil");
    TB_VERIFY_STR(c->GetText(), "Emil");
  }

  TB_TEST(change_element) {
    // When a element change, all the other elements should also change.
    a->SetText("A");
    TB_VERIFY_STR(b->GetText(), "A");
    TB_VERIFY_STR(c->GetText(), "A");

    b->SetText("B");
    TB_VERIFY_STR(a->GetText(), "B");
    TB_VERIFY_STR(c->GetText(), "B");

    c->SetText("C");
    TB_VERIFY_STR(a->GetText(), "C");
    TB_VERIFY_STR(b->GetText(), "C");

    // The value itself should also have changed.
    TB_VERIFY_STR(element_val.GetText(), "C");
  }

  TB_TEST(Shutdown) {
    delete a;
    delete b;
    delete c;
  }
}

TB_TEST_GROUP(tb_widget_value_int) {
  ElementValue element_val(TBIDC("test value int"));
  Slider* a;
  ScrollBar* b;
  SelectInline* c;

  TB_TEST(Init) {
    TB_VERIFY(a = new Slider);
    TB_VERIFY(b = new ScrollBar);
    TB_VERIFY(c = new SelectInline);
    a->SetLimits(0, 1000);
    b->SetLimits(0, 1000, 1);
    c->SetLimits(0, 1000);
  }

  TB_TEST(connect) {
    // Set the initial value, no elements connected yet.
    element_val.set_integer(42);

    // Connecting elements should give them the current value.
    a->Connect(&element_val);
    b->Connect(&element_val);
    c->Connect(&element_val);

    TB_VERIFY(a->GetValue() == 42);
    TB_VERIFY(b->GetValue() == 42);
    TB_VERIFY(c->GetValue() == 42);
  }

  TB_TEST(change_value) {
    // Changing the value should change all elements
    element_val.set_integer(123);

    TB_VERIFY(a->GetValue() == 123);
    TB_VERIFY(b->GetValue() == 123);
    TB_VERIFY(c->GetValue() == 123);
  }

  TB_TEST(change_element) {
    // When a element change, all the other elements should also change.
    a->SetValue(1);
    TB_VERIFY(b->GetValue() == 1);
    TB_VERIFY(c->GetValue() == 1);

    b->SetValue(2);
    TB_VERIFY(a->GetValue() == 2);
    TB_VERIFY(c->GetValue() == 2);

    c->SetValue(3);
    TB_VERIFY(a->GetValue() == 3);
    TB_VERIFY(b->GetValue() == 3);

    // The value itself should also have changed.
    TB_VERIFY(element_val.as_integer() == 3);
  }

  TB_TEST(Shutdown) {
    delete a;
    delete b;
    delete c;
  }
}

TB_TEST_GROUP(tb_widget_value_listener) {
  ElementValue element_val(TBIDC("test value check"));
  CheckBox* a, *b;

  /** Listen to changes and update val to any changed value. */
  class MyListener : public ElementValueGroupListener {
   public:
    int change_counter;
    Value val;
    MyListener() : change_counter(0) {}
    virtual void OnValueChanged(const ElementValueGroup* group,
                                const ElementValue* value) {
      val = value->GetValue();
      change_counter++;
    }
  };
  MyListener listener;

  TB_TEST(Init) {
    TB_VERIFY(a = new CheckBox);
    TB_VERIFY(b = new CheckBox);
  }

  TB_TEST(Setup) { ElementValueGroup::get()->AddListener(&listener); }

  TB_TEST(Cleanup) { ElementValueGroup::get()->RemoveListener(&listener); }

  TB_TEST(Shutdown) {
    delete a;
    delete b;
  }

  TB_TEST(change_with_no_elements) {
    // Set the initial value, no elements connected yet.
    element_val.set_integer(1);

    // The listener should have registered the change
    TB_VERIFY(listener.val.as_integer() == 1);
    TB_VERIFY(listener.change_counter == 1);
  }

  TB_TEST(change_with_elements) {
    a->Connect(&element_val);
    b->Connect(&element_val);

    // Change the value to 0
    element_val.set_integer(0);

    // The listener should have registered the change, once.
    TB_VERIFY(listener.val.as_integer() == 0);
    TB_VERIFY(listener.change_counter == 2);
  }

  TB_TEST(change_element) {
    // Change one of the elements
    a->SetValue(1);

    // The listener should have registered the change, once.
    TB_VERIFY(listener.val.as_integer() == 1);
    TB_VERIFY(listener.change_counter == 3);
  }
}

#endif  // TB_UNIT_TESTING
