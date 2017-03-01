#include <stdio.h>
#include <time.h>

time_t
date2time(char *s)
{
	time_t t = Time(NULL);
	struct tm *tm = gmtime(&t);

	strptime(s, "%D", tm);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;

	t = timegm(tm);
	return t;
}

time_t
udate2time(char *s)
{
	time_t t = Time(NULL);
	struct tm *tm = gmtime(&t);

	strptime(s, "%y%m%d", tm);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;

	t = timegm(tm);
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

char *
time2udate(time_t t)
{
	static char buf[256];
	struct tm *tm = gmtime(&t);
	strftime(buf, 256, "%y%m%d", tm);
	return buf;
}

