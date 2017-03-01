#include <stdio.h>
#include <time.h>
#include <strings.h>

time_t
date2time(char *s)
{
	time_t t;
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
	struct tm *tm = gmtime(&t);
	strftime(buf, 256, "%D", tm);
	return buf;
}
