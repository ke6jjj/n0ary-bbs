#include "test.h"
#include "tests.h"
#include <stdarg.h>
#include <stdio.h>
#include <board/uart.h>
#include <board/rcc.h>

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
#if 0
static size_t uartf_write(void *opaque, const char *buf, size_t sz);
#endif

void
start(void)
{
#if 0
  uart_handle uart;
  
  /* Set up the clock tree */
  rcc_clock_frequency_note(CLK_HSE, 8000000);
  
  /*
   * Open channel 1 (USART2).
   */
  uart_init(1, &uart);
  
  /*
   * Configure for 57600 baud.
   */
  uart_set_baud(uart, 57600);
  
  /*
   * Set up for 8N1.
   */
  uart_set_frame(uart, 8, UART_PARITY_NONE, UART_STOP_1);
  
  /*
   * Enable transmission.
   */
  uart_enable_tx(uart, 1);
  
  /* Set up a file handle for the uart */
  funopen(stdout, uart, uartf_write, NULL, NULL, NULL);
#endif
  
  RUN_TESTS();
}

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
RUN_TESTS(void)
{
  Local_Tests_verbose = 1;
  
  TESTSUITE_BEGIN();
  
  test_libc_printf();
  test_libc_strtoi();
  test_libc_itostr();
  test_ulib_malloc();
  test_board_rcc();
  test_board_isr();
  test_board_rtc();
  test_board_gpio();
  test_board_uart();
  
  TESTSUITE_END();
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

#if 0
static size_t
uartf_write(void *opaque, const char *buf, size_t sz)
{
  size_t i;
  uart_handle dev = (uart_handle) opaque;
  
  for (i = 0; i < sz; i++) {
    if (buf[i] == '\n')
      uart_putc(dev, '\r');
    uart_putc(dev, buf[i]);
  }
  
  return sz;
}
#endif
