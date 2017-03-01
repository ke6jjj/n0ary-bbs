#include <stdio.h>
#include <time.h>
#include <strings.h>

#include "date.h"

time_t
date2time(char *s)
{
	long t;
	struct tm tm;

	bzero(&tm, sizeof(tm));
	strptime(s, "%D", &tm);

#ifdef HAVE_TIMEGM
	t = timegm(&tm);
#else
	t = mktime(&tm);
#endif
	return t;
}

char *
time2date(time_t t)
{
	static char buf[256];
	struct tm ltm, *tm = gmtime_r(&t, &ltm);
	strftime(buf, 256, "%D", tm);
	return buf;
}
