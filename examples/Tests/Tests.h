#ifndef __TEST_H__
#define __TEST_H__
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Streaming.h"

static int test_passed = 0;
static int test_failed = 0;

/* Terminate current test with error */
#define fail()  return __LINE__

/* Successfull end of the test case */
#define done() return 0

/* Check single condition */
#define check(cond) do { if (!(cond)) fail(); } while (0)

/* Test runner */
static void test(int (*func)(void), const char *name)
{
  int r = func();
  if (r == 0)
  {
    test_passed++;
  }
  else
  {
    test_failed++;
    Serial << "FAILED: " << name << "(at line " << r << "\n";
    // printf("FAILED: %s (at line %d)\n", name, r);
  }
}

#endif /* __TEST_H__ */
