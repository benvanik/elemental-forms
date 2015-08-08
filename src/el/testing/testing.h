/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TESTING_TESTING_H_
#define EL_TESTING_TESTING_H_

/**
        This file contains a very simple unit testing framework.

        There are more capable unit testing frameworks available (that
        might be more suitable if you need extensive testing of your target
        application.

        I've chosen to not use any other framework for internal testing of
        Turbo Badger to minimize dependences.

        Test groups and tests:
        ---------------------

        Tests are specified in named groups, that can contain multiple named
   tests
        that are run in the order specified, except for special tests.

        Special tests are test with the names Init, Shutdown, Setup, Cleanup.
        They can be left out if they are not needed.

        Init and Shutdown - Called once per group. If Init fail, the tests in
   the
                                                group and Shutdown will be
   skipped.
        Setup and Cleanup - Called once per test. If Setup fail, the current
   test
                                                and Cleanup will be skipped.

        How to define a single test:
        ---------------------------

        EL_TEST_GROUP(groupname)
        {
                EL_TEST(testname)
                {
                        // Here goes test code and calls
                        // to EL_VERIFY, EL_PASS, EL_FAIL etc.
                        EL_VERIFY(1);
                }
        }

        How to define multiple tests, with data, setup and cleanup:
        ----------------------------------------------------------

        EL_TEST_GROUP(groupname)
        {
                // Here goes the data for this group
                std::string str;

                // Here goes methods with access to data
                bool is_str_empty() { return str.empty(); }

                // A test with name Setup will be called before each test.
                // If it fail, no other tests will be called (not even Cleanup).
                EL_TEST(Setup)
                {
                        // Setup
                }

                // The actual test code. Will be called if Setup passed.
                EL_TEST(test_something_1)
                {
                        // Test 1
                }

                // Another test code. Will be called if Setup passed.
                EL_TEST(test_something_2)
                {
                        // Test 2
                }

                // A test with name Cleanup will be called after each test.
                // Will be called even if the test failed.
                EL_TEST(Cleanup)
                {
                        // Cleanup
                }
        }
*/

#include <cmath>
#include <cstdint>
#include <string>

#include "el/config.h"
#include "el/types.h"
#include "el/util/intrusive_list.h"

namespace el {

/** Setting for TBRunTests to print out more information. */
#define EL_TEST_VERBOSE 1

#ifdef EL_UNIT_TESTING

/** Run the tests. Return the number of fails. */
int TBRunTests(uint32_t settings = EL_TEST_VERBOSE);

/** Verify that the expression is true and fail if it isn't. */
#define EL_VERIFY(expr)      \
  {                          \
    fail_line_nr = __LINE__; \
    fail_file = __FILE__;    \
    if (!(expr)) {           \
      fail_text = (#expr);   \
      return;                \
    }                        \
  }

/** Verify that the values are approximately the same. */
#define EL_VERIFY_FLOAT(val, ref_val) \
  { EL_VERIFY(fabs(ref_val - val) < 1.0E-5); }

/** Verify that the strings are equal. */
#define EL_VERIFY_STR(str1, str2) \
  { EL_VERIFY(std::string(str1).compare(str2) == 0); }

/** End the test with a pass. */
#define EL_PASS() return;

/** End the test with a description why it failed. */
#define EL_FAIL(error)       \
  {                          \
    fail_line_nr = __LINE__; \
    fail_file = __FILE__;    \
    fail_text = error;       \
    return;                  \
  }

/** Return a absolute path for the given filename relative to the test source
 * file. */
#define EL_TEST_FILE(filename) tb_get_test_file_name(__FILE__, filename)

/** TBCall is used to execute callbacks for tests in TBTestGroup. */
class TBCall : public util::IntrusiveListEntry<TBCall> {
 public:
  /** return the name of the call */
  virtual const char* name() = 0;
  /** execute the test code */
  virtual void exec() = 0;
};

/** TBTestGroup has a collection of callbacks for tests, and optional Setup and
 * Cleanup calls. */
class TBTestGroup {
 public:
  TBTestGroup(const char* name);
  bool IsSpecialTest(TBCall* call) const { return !call->linklist; }

 public:
  const char* name;                   // Test group name.
  TBCall* setup;                      // Setup call, or nullptr.
  TBCall* cleanup;                    // Cleanup call, or nullptr.
  TBCall* init;                       // Init call, or nullptr.
  TBCall* shutdown;                   // Shutdown call, or nullptr.
  util::IntrusiveList<TBCall> calls;  // All test calls to call.
  TBTestGroup* next_test_group;
};

/** TBRegisterCall is used for registering calls on TBTestGroup .*/
class TBRegisterCall {
 public:
  TBRegisterCall(TBTestGroup* test, TBCall* call);
  ~TBRegisterCall();

 private:
  TBCall* call;
};

#define EL_TEST_GROUP(name)               \
  namespace testgroup_##name {            \
    class TheGroup : public TBTestGroup { \
     public:                              \
      TheGroup() : TBTestGroup(#name) {}  \
    };                                    \
    TheGroup the_group_obj;               \
    int force_link = 0;                   \
  }                                       \
  namespace testgroup_##name

#define EL_TEST(callname)                                     \
  class CallObj##callname : public TBCall {                   \
   public:                                                    \
    virtual const char* name();                               \
    virtual void exec();                                      \
  };                                                          \
  CallObj##callname callname;                                 \
  TBRegisterCall callname##reg(&the_group_obj, &callname);    \
  const char* CallObj##callname::name() { return #callname; } \
  void CallObj##callname::exec()

/** Hack to force linking the given test group.
        This might be needed for test groups in source files that contain
        nothing else that's referenced by the application, and which are
        linked in an library. */
#define EL_FORCE_LINK_TEST_GROUP(name) \
  namespace testgroup_##name {         \
    void force_link_call() {           \
      extern int force_link;           \
      force_link = 1;                  \
    }                                  \
  }

std::string tb_get_test_file_name(const char* testpath, const char* filename);

extern uint32_t test_settings;  // Settings, as sent to TBRunTests.
extern int fail_line_nr;        // Fail line number.
extern const char* fail_file;   // Fail file name.
extern const char* fail_text;   // Fail text description.

#else

inline int TBRunTests(uint32_t settings = EL_TEST_VERBOSE) { return 0; }

#endif

}  // namespace el

#endif  // EL_TESTING_TESTING_H_
