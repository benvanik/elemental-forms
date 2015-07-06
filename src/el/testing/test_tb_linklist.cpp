/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;

EL_TEST_GROUP(tb_linklist) {
  class Apple : public util::IntrusiveListEntry<Apple> {
   public:
    Apple() : id(0) { total_apple_count++; }
    Apple(int id) : id(id) { total_apple_count++; }
    ~Apple() { total_apple_count--; }
    void eat() {}

    int id;
    static int total_apple_count;
  };
  int Apple::total_apple_count = 0;

  util::AutoDeleteIntrusiveList<Apple> list;

  bool AddAppless(int num_apples) {
    for (int i = 0; i < num_apples; i++) list.AddLast(new Apple(i + 1));
    return list.CountLinks() == num_apples;
  }

  EL_TEST(Setup) {
    // Add 3 apples to list for each test.
    list.DeleteAll();
    EL_VERIFY(AddAppless(3));
  }

  EL_TEST(Shutdown) { list.DeleteAll(); }

  EL_TEST(iteration_while_delete_all) {
    auto iterator = list.IterateForward();
    while (Apple* apple = iterator.GetAndStep()) {
      apple->eat();
      // Lets pretend we do something with apple
      // that trig deletion of all apples.
      list.DeleteAll();
    }
    // success already, if we didn't crash
    EL_VERIFY(list.CountLinks() == 0);
  }

  EL_TEST(iteration_while_delete) {
    auto iterator = list.IterateForward();
    while (Apple* apple = iterator.GetAndStep()) {
      // Lets pretend we do something with apple
      // that trig deletion of both apple and next apple!
      if (apple->GetNext()) list.Delete(apple->GetNext());
      list.Delete(apple);
    }
    // success already, if we didn't crash
    EL_VERIFY(list.CountLinks() == 0);
  }

  EL_TEST(iteration_while_list_delete) {
    // Allocate a list that does not own its apples.
    // We need the apples to survive their list to test this fully.
    auto apple_refs = new util::IntrusiveList<Apple>;
    Apple apples[3];
    for (int i = 0; i < 3; i++) apple_refs->AddLast(&apples[i]);

    EL_VERIFY(apples[0].IsInList());
    EL_VERIFY(apples[1].IsInList());
    EL_VERIFY(apples[2].IsInList());

    // Lets pretend we do some iteration in a list...
    auto iterator = apple_refs->IterateForward();
    EL_VERIFY(iterator.get());

    // Now the list itself gets deallocated.
    delete apple_refs;

    // Since the list is gone, the links should not be added anymore.
    EL_VERIFY(!apples[0].IsInList());
    EL_VERIFY(!apples[1].IsInList());
    EL_VERIFY(!apples[2].IsInList());

    // Now the iterator should not point to anything,
    // and we should not crash.
    EL_VERIFY(!iterator.get());
  }

  EL_TEST(forward_iterator) {
    auto i = list.IterateForward();
    EL_VERIFY(i.get()->id == 1);

    EL_VERIFY(i.GetAndStep()->id == 1);
    EL_VERIFY(i.GetAndStep()->id == 2);
    EL_VERIFY(i.GetAndStep()->id == 3);
    EL_VERIFY(i.get() == nullptr);

    i.Reset();
    EL_VERIFY(i.get()->id == 1);
  }

  EL_TEST(backward_iterator) {
    auto i = list.IterateBackward();
    EL_VERIFY(i.get()->id == 3);

    EL_VERIFY(i.GetAndStep()->id == 3);
    EL_VERIFY(i.GetAndStep()->id == 2);
    EL_VERIFY(i.GetAndStep()->id == 1);
    EL_VERIFY(i.get() == nullptr);

    i.Reset();
    EL_VERIFY(i.get()->id == 3);
  }

  EL_TEST(multiple_iterators_assign) {
    auto iA = list.IterateForward();
    auto iB = list.IterateBackward();

    EL_VERIFY(iA.get()->id == 1);
    EL_VERIFY(iB.get()->id == 3);

    iA = iB;
    EL_VERIFY(iA.GetAndStep()->id == 3);
    EL_VERIFY(iA.GetAndStep()->id == 2);
    EL_VERIFY(iA.GetAndStep()->id == 1);
  }

  EL_TEST(multiple_iterators_assign_swap_list) {
    util::AutoDeleteIntrusiveList<Apple> other_list;
    other_list.AddLast(new Apple(42));

    auto iA = list.IterateForward();
    auto iB = other_list.IterateForward();

    EL_VERIFY(iA.get()->id == 1);
    EL_VERIFY(iB.get()->id == 42);

    iA = iB;
    EL_VERIFY(iA.GetAndStep()->id == 42);
    EL_VERIFY(iA.get() == nullptr);
  }

  EL_TEST(autodelete) {
    // Check that the apples really are destroyed.
    int old_total_apple_count = Apple::total_apple_count;
    // Scope for util::AutoDeleteIntrusiveList
    {
      util::AutoDeleteIntrusiveList<Apple> autodelete_list;
      autodelete_list.AddLast(new Apple(1));
      autodelete_list.AddLast(new Apple(2));
      EL_VERIFY(Apple::total_apple_count == old_total_apple_count + 2);
    }
    EL_VERIFY(Apple::total_apple_count == old_total_apple_count);
  }
}

#endif  // EL_UNIT_TESTING
