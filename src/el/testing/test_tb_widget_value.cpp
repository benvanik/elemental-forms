/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/element_value.h"
#include "el/elements/check_box.h"
#include "el/elements/slider.h"
#include "el/elements/spin_box.h"
#include "el/elements/text_box.h"
#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using namespace el::elements;

EL_TEST_GROUP(tb_widget_value_text) {
  ElementValue element_val(TBIDC("test value text"));
  Element* a, *b, *c;

  EL_TEST(Init) {
    EL_VERIFY(a = new TextBox);
    EL_VERIFY(b = new TextBox);
    EL_VERIFY(c = new TextBox);
  }

  EL_TEST(connect) {
    // Set the initial value, no elements connected yet.
    element_val.set_text("elemental forms");

    // Connecting elements should give them the current value.
    a->Connect(&element_val);
    b->Connect(&element_val);
    c->Connect(&element_val);

    EL_VERIFY_STR(a->text(), "elemental forms");
    EL_VERIFY_STR(b->text(), "elemental forms");
    EL_VERIFY_STR(c->text(), "elemental forms");
  }

  EL_TEST(change_value) {
    // Changing the value should change all elements
    element_val.set_text("Emil");

    EL_VERIFY_STR(a->text(), "Emil");
    EL_VERIFY_STR(b->text(), "Emil");
    EL_VERIFY_STR(c->text(), "Emil");
  }

  EL_TEST(change_element) {
    // When a element change, all the other elements should also change.
    a->set_text("A");
    EL_VERIFY_STR(b->text(), "A");
    EL_VERIFY_STR(c->text(), "A");

    b->set_text("B");
    EL_VERIFY_STR(a->text(), "B");
    EL_VERIFY_STR(c->text(), "B");

    c->set_text("C");
    EL_VERIFY_STR(a->text(), "C");
    EL_VERIFY_STR(b->text(), "C");

    // The value itself should also have changed.
    EL_VERIFY_STR(element_val.text(), "C");
  }

  EL_TEST(Shutdown) {
    delete a;
    delete b;
    delete c;
  }
}

EL_TEST_GROUP(tb_widget_value_int) {
  ElementValue element_val(TBIDC("test value int"));
  Slider* a;
  ScrollBar* b;
  SpinBox* c;

  EL_TEST(Init) {
    EL_VERIFY(a = new Slider);
    EL_VERIFY(b = new ScrollBar);
    EL_VERIFY(c = new SpinBox);
    a->set_limits(0, 1000);
    b->set_limits(0, 1000, 1);
    c->set_limits(0, 1000);
  }

  EL_TEST(connect) {
    // Set the initial value, no elements connected yet.
    element_val.set_integer(42);

    // Connecting elements should give them the current value.
    a->Connect(&element_val);
    b->Connect(&element_val);
    c->Connect(&element_val);

    EL_VERIFY(a->value() == 42);
    EL_VERIFY(b->value() == 42);
    EL_VERIFY(c->value() == 42);
  }

  EL_TEST(change_value) {
    // Changing the value should change all elements
    element_val.set_integer(123);

    EL_VERIFY(a->value() == 123);
    EL_VERIFY(b->value() == 123);
    EL_VERIFY(c->value() == 123);
  }

  EL_TEST(change_element) {
    // When a element change, all the other elements should also change.
    a->set_value(1);
    EL_VERIFY(b->value() == 1);
    EL_VERIFY(c->value() == 1);

    b->set_value(2);
    EL_VERIFY(a->value() == 2);
    EL_VERIFY(c->value() == 2);

    c->set_value(3);
    EL_VERIFY(a->value() == 3);
    EL_VERIFY(b->value() == 3);

    // The value itself should also have changed.
    EL_VERIFY(element_val.as_integer() == 3);
  }

  EL_TEST(Shutdown) {
    delete a;
    delete b;
    delete c;
  }
}

EL_TEST_GROUP(tb_widget_value_listener) {
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

  EL_TEST(Init) {
    EL_VERIFY(a = new CheckBox);
    EL_VERIFY(b = new CheckBox);
  }

  EL_TEST(Setup) { ElementValueGroup::get()->AddListener(&listener); }

  EL_TEST(Cleanup) { ElementValueGroup::get()->RemoveListener(&listener); }

  EL_TEST(Shutdown) {
    delete a;
    delete b;
  }

  EL_TEST(change_with_no_elements) {
    // Set the initial value, no elements connected yet.
    element_val.set_integer(1);

    // The listener should have registered the change
    EL_VERIFY(listener.val.as_integer() == 1);
    EL_VERIFY(listener.change_counter == 1);
  }

  EL_TEST(change_with_elements) {
    a->Connect(&element_val);
    b->Connect(&element_val);

    // Change the value to 0
    element_val.set_integer(0);

    // The listener should have registered the change, once.
    EL_VERIFY(listener.val.as_integer() == 0);
    EL_VERIFY(listener.change_counter == 2);
  }

  EL_TEST(change_element) {
    // Change one of the elements
    a->set_value(1);

    // The listener should have registered the change, once.
    EL_VERIFY(listener.val.as_integer() == 1);
    EL_VERIFY(listener.change_counter == 3);
  }
}

#endif  // EL_UNIT_TESTING
