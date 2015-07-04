/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"
#include "tb/util/object.h"

#ifdef TB_UNIT_TESTING

using namespace tb;
using namespace tb::util;

TB_TEST_GROUP(tb_object) {
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

  TB_TEST(safe_cast) {
    Fruit fruit;
    Apple apple;
    Car car;

    TB_VERIFY(SafeCast<TypedObject>(&fruit));
    TB_VERIFY(SafeCast<TypedObject>(&apple));
    TB_VERIFY(SafeCast<TypedObject>(&car));

    TB_VERIFY(SafeCast<Fruit>(&fruit));
    TB_VERIFY(SafeCast<Fruit>(&apple));
    TB_VERIFY(!SafeCast<Fruit>(&car));

    TB_VERIFY(!SafeCast<Apple>(&fruit));
    TB_VERIFY(SafeCast<Apple>(&apple));
    TB_VERIFY(!SafeCast<Apple>(&car));

    TB_VERIFY(!SafeCast<Car>(&fruit));
    TB_VERIFY(!SafeCast<Car>(&apple));
    TB_VERIFY(SafeCast<Car>(&car));
  }
}

#endif  // TB_UNIT_TESTING
