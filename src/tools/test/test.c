#include <stdarg.h>
#include <stdio.h>

#include "test.h"

static unsigned int Tests_passed;
static unsigned int Tests_failed;
static unsigned int Local_Tests_passed;
static unsigned int Local_Tests_failed;
static unsigned int Local_Tests_number;
static const char * Local_Tests_name;
static int          Local_Tests_verbose;

static void TEST_PASSED(void);
static void TEST_FAILED(void);
static void RUN_TESTS(void);

void
TESTSUITE_BEGIN(void)
{
  Tests_passed = 0;
  Tests_failed = 0;
}

void
TEST_BEGIN(const char *name)
{
  printf("Starting test %s...\n", name);
  Local_Tests_name = name;
  Local_Tests_passed = 0;
  Local_Tests_failed = 0;
  Local_Tests_number = 1;
}

void
TEST_RESULT(int result)
{
  if (result)
    TEST_PASSED();
  else
    TEST_FAILED();
  Local_Tests_number++;
}

void
TEST_END(void)
{
  if (Local_Tests_failed == 0)
    printf("PASSED %s (%d)\n", Local_Tests_name, Local_Tests_passed);
  else
    printf("FAILED %s (%d/%d) passed.\n", Local_Tests_name, Local_Tests_passed,
         Local_Tests_passed + Local_Tests_failed);
  Tests_passed += Local_Tests_passed;
  Tests_failed += Local_Tests_failed;
}

void
TESTSUITE_END(void)
{
  if (Tests_failed == 0)
    printf("PASSED all tests.\n");
  else
    printf("FAILED some tests.\n");
}

static void
TEST_PASSED(void)
{
  if (Local_Tests_verbose)
    printf("...%d passed.\n", Local_Tests_number);
  Local_Tests_passed++;
}

static void
TEST_FAILED(void)
{
  if (Local_Tests_verbose)
    printf("...%d failed.\n", Local_Tests_number);
  Local_Tests_failed++;
}
