/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"
#include "tb/element_value.h"
#include "tb/elements/check_box.h"
#include "tb/elements/slider.h"
#include "tb/elements/spin_box.h"
#include "tb/elements/text_box.h"

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
    element_val.set_text("turbo badger");

    // Connecting elements should give them the current value.
    a->Connect(&element_val);
    b->Connect(&element_val);
    c->Connect(&element_val);

    TB_VERIFY_STR(a->text(), "turbo badger");
    TB_VERIFY_STR(b->text(), "turbo badger");
    TB_VERIFY_STR(c->text(), "turbo badger");
  }

  TB_TEST(change_value) {
    // Changing the value should change all elements
    element_val.set_text("Emil");

    TB_VERIFY_STR(a->text(), "Emil");
    TB_VERIFY_STR(b->text(), "Emil");
    TB_VERIFY_STR(c->text(), "Emil");
  }

  TB_TEST(change_element) {
    // When a element change, all the other elements should also change.
    a->set_text("A");
    TB_VERIFY_STR(b->text(), "A");
    TB_VERIFY_STR(c->text(), "A");

    b->set_text("B");
    TB_VERIFY_STR(a->text(), "B");
    TB_VERIFY_STR(c->text(), "B");

    c->set_text("C");
    TB_VERIFY_STR(a->text(), "C");
    TB_VERIFY_STR(b->text(), "C");

    // The value itself should also have changed.
    TB_VERIFY_STR(element_val.text(), "C");
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
  SpinBox* c;

  TB_TEST(Init) {
    TB_VERIFY(a = new Slider);
    TB_VERIFY(b = new ScrollBar);
    TB_VERIFY(c = new SpinBox);
    a->set_limits(0, 1000);
    b->set_limits(0, 1000, 1);
    c->set_limits(0, 1000);
  }

  TB_TEST(connect) {
    // Set the initial value, no elements connected yet.
    element_val.set_integer(42);

    // Connecting elements should give them the current value.
    a->Connect(&element_val);
    b->Connect(&element_val);
    c->Connect(&element_val);

    TB_VERIFY(a->value() == 42);
    TB_VERIFY(b->value() == 42);
    TB_VERIFY(c->value() == 42);
  }

  TB_TEST(change_value) {
    // Changing the value should change all elements
    element_val.set_integer(123);

    TB_VERIFY(a->value() == 123);
    TB_VERIFY(b->value() == 123);
    TB_VERIFY(c->value() == 123);
  }

  TB_TEST(change_element) {
    // When a element change, all the other elements should also change.
    a->set_value(1);
    TB_VERIFY(b->value() == 1);
    TB_VERIFY(c->value() == 1);

    b->set_value(2);
    TB_VERIFY(a->value() == 2);
    TB_VERIFY(c->value() == 2);

    c->set_value(3);
    TB_VERIFY(a->value() == 3);
    TB_VERIFY(b->value() == 3);

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
      val = value->value();
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
    a->set_value(1);

    // The listener should have registered the change, once.
    TB_VERIFY(listener.val.as_integer() == 1);
    TB_VERIFY(listener.change_counter == 3);
  }
}

#endif  // TB_UNIT_TESTING
