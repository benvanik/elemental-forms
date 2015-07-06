/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/testing/testing.h"
#include "el/util/object.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using namespace el::util;

EL_TEST_GROUP(tb_object) {
  class Car : public TypedObject {
   public:
    TBOBJECT_SUBCLASS(Car, TypedObject);
  };

  class Fruit : public TypedObject {
   public:
    TBOBJECT_SUBCLASS(Fruit, TypedObject);
  };

  class Apple : public Fruit {
   public:
    TBOBJECT_SUBCLASS(Apple, Fruit);
  };

  EL_TEST(safe_cast) {
    Fruit fruit;
    Apple apple;
    Car car;

    EL_VERIFY(SafeCast<TypedObject>(&fruit));
    EL_VERIFY(SafeCast<TypedObject>(&apple));
    EL_VERIFY(SafeCast<TypedObject>(&car));

    EL_VERIFY(SafeCast<Fruit>(&fruit));
    EL_VERIFY(SafeCast<Fruit>(&apple));
    EL_VERIFY(!SafeCast<Fruit>(&car));

    EL_VERIFY(!SafeCast<Apple>(&fruit));
    EL_VERIFY(SafeCast<Apple>(&apple));
    EL_VERIFY(!SafeCast<Apple>(&car));

    EL_VERIFY(!SafeCast<Car>(&fruit));
    EL_VERIFY(!SafeCast<Car>(&apple));
    EL_VERIFY(SafeCast<Car>(&car));
  }
}

#endif  // EL_UNIT_TESTING
