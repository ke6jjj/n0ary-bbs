#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "wp.h"

int
udate2time(char *s, time_t *res)
{
	int year, mon, day;
	struct tm tm;
	size_t len;

	len = strlen(s);

	if (len < 6) {
		/* Not possible to parse */
		return -1;
	}

	if (len == 6) {
		/* YYMMDD - 2-digit year */
		day = atoi(&s[4]); s[4] = 0;
		mon = atoi(&s[2]); s[2] = 0;
		year = atoi(s);

		/*
		 * Interpret the date in a Y2K compliant manner.
		 * Assume that no one ever used a packet BBS before 1980.
		 * If this format is still in use by 2080, seek help.
		 */
		if (year < 80) {
			year += 100;
		}
	} else {
		/* Y*MMDD format (y10k and beyond safe) */
		day = atoi(&s[len-2]); s[len-2] = 0;
		mon = atoi(&s[len-4]); s[len-4] = 0;
		year = atoi(s);
		year -= 1900;
	}

	bzero(&tm, sizeof(tm));
	tm.tm_mday = day;
	tm.tm_mon = mon - 1;
	tm.tm_year = year;
	tm.tm_sec = 0;
	tm.tm_min = 1;
	tm.tm_hour = 0;

#ifdef HAVE_TIMEGM
	*res = timegm(&tm);
#else
	*res = mktime(&tm);
#endif

	return 0;
}

char *
time2udate(time_t t)
{
	static char buf[256];
	struct tm *tm = gmtime(&t);
	strftime(buf, 256, "%y%m%d", tm);
	return buf;
}
