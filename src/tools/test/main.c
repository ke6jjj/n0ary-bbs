#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_cmmn.h"
#include "test.h"
#include "tools.h"

struct ti_test {
	const char *input;
	long default_unit;
	int two_words;
	int exp_return;
	long exp_value;
	const char *exp_end;
};

const struct ti_test ti_tests[] = {
	{ "1 second",       tDay,    1, OK,    1,          "second"      },
	{ "1 minute",       tDay,    1, OK,    1*tMin,     "minute"      },
	{ "1 hour",         tDay,    1, OK,    1*tHour,    "hour"        },
	{ "1 week",         tDay,    1, OK,    1*tWeek,    "week"        },
	{ "1 month",        tDay,    1, OK,    1*tMonth ,  "month"       },
	{ "1",              tDay,    1, OK,    1*tDay,     ""            },
	{ "1year",          tDay,    0, OK,    1*tYear,    "year"        },
	{ "1year",          tDay,    1, OK,    1*tYear,    "year"        },
	{ "1 year",         tDay,    0, OK,    1*tDay,     " year"       },
	{ "1 blob",         tDay,    1, ERROR, 0,          NULL          },
	{ "17 seconds",     tDay,    1, OK,    17,         "seconds"     },
	{ "17 minutes",     tDay,    1, OK,    17*tMin,    "minutes"     },
	{ "17 hours",       tDay,    1, OK,    17*tHour,   "hours"       },
	{ "17 weeks",       tDay,    1, OK,    17*tWeek,   "weeks"       },
	{ "17 months",      tDay,    1, OK,    17*tMonth,  "months"      },
	{ "17 years",       tDay,    1, OK,    17*tYear,   "years"       },
	{ "17",             tYear,   1, OK,    17*tYear,   ""            },
	{ "",               tYear,   1, ERROR, 0,          NULL          },
};
const size_t ti_tests_count = sizeof(ti_tests) / sizeof(ti_tests[0]);
	
int
main(int argc, char *argv[])
{
	long t;
	char *p, *buf;
	int res;
	size_t i;

	TESTSUITE_BEGIN();

	for (i = 0; i < ti_tests_count; i++) {
		TEST_BEGIN(ti_tests[i].input);

		buf = strdup(ti_tests[i].input);
		p = buf;
		res = get_time_interval(
			&p,
			ti_tests[i].default_unit,
			ti_tests[i].two_words,
			&t);

		TEST_PASS_IF(res == ti_tests[i].exp_return);

		if (ti_tests[i].exp_return == OK && res == OK) {
			TEST_PASS_IF(t == ti_tests[i].exp_value);
			TEST_PASS_IF(strcmp(p, ti_tests[i].exp_end) == 0);
		}

		free(buf);
		TEST_END();
	}

	TESTSUITE_END();

	return 0;
}
