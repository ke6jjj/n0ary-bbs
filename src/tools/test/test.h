#ifndef TOOLS_TEST_H
#define TOOLS_TEST_H

void TESTSUITE_BEGIN(void);
void TEST_BEGIN(const char *name);
void TEST_RESULT(int pass);
void TEST_END(void);
void TESTSUITE_END(void);

#define TEST_PASS() TEST_RESULT(1)
#define TEST_FAIL() TEST_RESULT(0)
#define TEST_PASS_IF(cond) TEST_RESULT(cond)

#endif
