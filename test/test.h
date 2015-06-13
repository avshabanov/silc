#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef countof
#define countof(arr)    (sizeof(arr) / sizeof(arr[0]))
#endif

#ifdef NDEBUG
/* Ad hoc version of assert for release build */
#define ASSERT(x) \
  if (!(x)) { \
    fputs("Assertion failed: ", stderr); \
    fputs(#x, stderr); \
    fputc('\n', stderr); \
    abort(); \
  }
#else
#define ASSERT assert
#endif

#ifndef TEST_BEFORE
#define TEST_BEFORE() {}
#endif

#ifndef TEST_AFTER
#define TEST_AFTER() {}
#endif

#define BEGIN_TEST_METHOD(method_name) \
  static void method_name() { \
    TEST_BEFORE(); \
    const char * _test_succeeded_msg = "\rPASSED:   " #method_name "     \n"; \
    fputs("STARTING: " #method_name "...  ", stdout); fflush(stdout);

#define END_TEST_METHOD() \
    fputs(_test_succeeded_msg, stdout); \
    TEST_AFTER(); \
  }

#define TESTS_STARTED()             fputs("\nStarting tests from " __FILE__ " ...\n\n", stdout)
#define TESTS_SUCCEEDED()           fputs("\nSUCCESS:  all tests passed in " __FILE__ "\n\n", stdout)

/* Mem alloc for tests (never fails) */

static inline void * xmalloc(size_t size) {
  void * p = malloc(size);
  if (p == 0) {
    fputs("Out of memory\n", stderr);
    abort();
  }
  return p;
}

static inline void xfree(void * p) {
  free(p);
}
