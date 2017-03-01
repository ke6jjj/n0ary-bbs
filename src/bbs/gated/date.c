#include <stdio.h>
#include <time.h>

long
date2time(char *s)
{
	long t;
	struct tm tm;

	bzero(&tm, sizeof(tm));
	strptime(s, "%D", &tm);

#ifdef SUNOS
	t = timegm(&tm);
#else
	t = mktime(&tm);
#endif
	return t;
}

char *
time2date(long t)
{
	static char buf[256];
	struct tm *tm = gmtime(&t);
	strftime(buf, 256, "%D", tm);
	return buf;
}
