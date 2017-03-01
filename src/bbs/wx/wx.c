#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include "defaults.h"
#include "wx.h"

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"

#define byTNC	0
#define byPHONE	1
#define VIA		byTNC

#define PHONE	"9972973"

#define OUTDOOR		0
#define INDOOR		1

int
	Barometer, BarometerRate,
	Humidity, HumidityRate,
	HumidityIn, HumidityInRate,
	Rain, RainRate,
	IndoorTemp, IndoorTempRate,
	OutdoorTemp, OutdoorTempRate,
	WindChill, GustChill;

struct Hi_Low {
	int value;
	char t_d[20];
}
	BarometerHigh, BarometerLow,
	HumidityHigh, HumidityLow,
	HumidityInHigh, HumidityInLow,
	IndoorTempHigh, IndoorTempLow,
	OutdoorTempHigh, OutdoorTempLow;

struct WindStruct {
	int speed;
	int direction;
}
	Wind, Gust;

struct Wind_Hi_Low {
	struct WindStruct v;
	char t_d[20];
}
	WindHigh, WindLow,
	GustHigh, GustLow;

int
	ignore(), rd_time(), rd_date(), dummy(),
	single_param(), hi_low_param(), 
	wind_param(), wind_hi_low_param();

char stime[10], sdate[10];

struct Dispatch {
	char *cmd;
	int clen;
	char *rsp;
	int rlen;
	int (*func)();
	int *value;
} dispatch[] = {

#if 0
	{ "ATXCA\nATVD/\nATVT:\nATRD\n",	23, NULL, 0, rd_date, 0 },
#else
	{ "ATXCA\n",	6, NULL,	0, ignore, 0 },
	{ "ATVD/\n",	6, NULL,	0, ignore, 0 },
	{ "ATVT:\n",	6, NULL,	0, ignore, 0 },
	{ "ATRD\n",		5, NULL,	0, rd_date, 0 },
#endif
	{ "ATRT\n",		5, NULL,	0, rd_time, 0 },

	{ "ATRB\n",		5, "B",		1, single_param, &Barometer },
	{ "ATRBH\n",	6, "<B",	2, hi_low_param, (int*)&BarometerHigh },
	{ "ATRBL\n",	6, ">B",	2, hi_low_param, (int*)&BarometerLow },
	{ "ATRBr\n",	6, "Br",	2, single_param, &BarometerRate },

	{ "ATRHO\n",	6, "H",		1, single_param, &Humidity },
	{ "ATRHOH\n",	7, "<H",	2, hi_low_param, (int*)&HumidityHigh }, 
	{ "ATRHOL\n",	7, ">H",	2, hi_low_param, (int*)&HumidityLow },
	{ "ATRHOr\n",	7, "Hr",	2, single_param, &HumidityRate },

	{ "ATRHI\n",	6, "h",		1, single_param, &HumidityIn },
	{ "ATRHIH\n",	7, "<h",	2, hi_low_param, (int*)&HumidityInHigh }, 
	{ "ATRHIL\n",	7, ">h",	2, hi_low_param, (int*)&HumidityInLow },
	{ "ATRHIr\n",	7, "hr",	2, single_param, &HumidityInRate },

	{ "ATRR\n",		5, "R",		1, single_param, &Rain },
	{ "ATRRI\n",	6, "RI",	2, single_param, &RainRate },

	{ "ATRTI\n",	6, "t",		1, single_param, &IndoorTemp },
	{ "ATRTIH\n",	7, "<t",	2, hi_low_param, (int*)&IndoorTempHigh },
	{ "ATRTIL\n",	7, ">t",	2, hi_low_param, (int*)&IndoorTempLow },
	{ "ATRTIr\n",	7, "tr",	2, single_param, &IndoorTempRate },

	{ "ATRTO\n",	6, "T",		1, single_param, &OutdoorTemp },
	{ "ATRTOH\n",	7, "<T",	2, hi_low_param, (int*)&OutdoorTempHigh },
	{ "ATRTOL\n",	7, ">T",	2, hi_low_param, (int*)&OutdoorTempLow },
	{ "ATRTOr\n",	7, "Tr",	2, single_param, &OutdoorTempRate },

	{ "ATRWA\n",	6, "w",		1, wind_param, (int*)&Wind },
	{ "ATRWAH\n",	7, "<w",	2, wind_hi_low_param, (int*)&WindHigh },
#if 0
	{ "ATRWAL\n",	7, ">w",	2, wind_hi_low_param, (int*)&WindLow },
#endif
	{ "ATRWCA\n",	7, "cT",	2, single_param, &WindChill },
	{ "ATRWCG\n",	7, "CT",	2, single_param, &GustChill},
	{ "ATRWG\n",	6, "W",		1, wind_param, (int*)&Gust },
	{ "ATRWGH\n",	7, "<W",	2, wind_hi_low_param, (int*)&GustHigh },
#if 0
	{ "ATRWGL\n",	7, ">W",	2, wind_hi_low_param, (int*)&GustLow },
#endif
	{ NULL, 0, NULL, 0, NULL, 0 }};

struct Dispatch
	dispatch_clr[] = {
	{ "ATCBH\n",	6, NULL, 0, ignore, 0 },
	{ "ATCBL\n",	6, NULL, 0, ignore, 0 },
	{ "ATCHIH\n",	7, NULL, 0, ignore, 0 },
	{ "ATCHIL\n",	7, NULL, 0, ignore, 0 },
	{ "ATCHOH\n",	7, NULL, 0, ignore, 0 },
	{ "ATCHOL\n",	7, NULL, 0, ignore, 0 },
	{ "ATCTIH\n",	7, NULL, 0, ignore, 0 },
	{ "ATCTIL\n",	7, NULL, 0, ignore, 0 },
	{ "ATCTOH\n",	7, NULL, 0, ignore, 0 },
	{ "ATCTOL\n",	7, NULL, 0, ignore, 0 },
	{ "ATCWAH\n",	7, NULL, 0, ignore, 0 },
	{ "ATCWAL\n",	7, NULL, 0, ignore, 0 },
	{ "ATCWGH\n",	7, NULL, 0, ignore, 0 },
	{ "ATCWGL\n",	7, NULL, 0, ignore, 0 },
	{ NULL, 0, NULL, 0, NULL, 0 }};


char *get_response();

int fd;
FILE *log = NULL;

main(argc, argv)
int argc;
char *argv[];
{
	int clear_wx = FALSE;

	bbsd_check_in("WX", ServerPort);
	if(argc > 1)
		if(!strcmp(argv[1], "clear"))
			clear_wx = TRUE;

	bbsd_message("connecting to WA6YCZ");
	collect_weather(clear_wx);

	if(clear_wx)
		age_weather();

	write_weather(OUTDOOR);
	write_weather(INDOOR);

	write_weather_raw();

	wx_log("Successful run");
	if(log)
		fclose(log);

	bbsd_check_out();
	exit(0);
}

#define RETRYCNT	20		/* 20 retrys at 30 sec intervals */

collect_weather(clear_wx)
int clear_wx;
{
	struct Dispatch *d = dispatch;
	char buf[80];
	int retry = RETRYCNT;


	if(VIA == byPHONE)
		fd = open_phone(PHONE);
	else
#if 0
		fd = open_tnc(TNC2_PORT);
#endif
		fd = tnc_connect(BBS_HOST, TD[PD[2].indx].port, "WA6YCZ", "N0ARY-1");

	if(fd == ERROR) { 
		write_blank_record("Failed open_device call");
		fclose(log);
		exit(1);
	}

	while(d->clen) {
		char *s;

		write(fd, d->cmd, d->clen);
		{
			char dbuf[80];
			strcpy(dbuf, d->cmd);
			dbuf[strlen(dbuf)-1] = 0;
			bbsd_message(dbuf);
		}

		if((s = get_response()) == NULL) {
			if(--retry == 0) {
				write_blank_record("Failure communicating with the weather station");
				close(fd);
				fclose(log);
				exit(1);
			}
			continue;
		}

		if(d->func(d->rsp, s, d->rlen, d->value)) {
			write_blank_record("Failure communicating with the weather station");
			close(fd);
			fclose(log);
			exit(1);
		}

		retry = RETRYCNT;
		d++;
	}

	if(clear_wx) {
		d = dispatch_clr;
		while(d->clen) {
			write(fd, d->cmd, d->clen);
			{
				char dbuf[80];
				strcpy(dbuf, d->cmd);
				dbuf[strlen(dbuf)-1] = 0;
				bbsd_message(dbuf);
			}
			if(get_response() == NULL) {
				if(--retry == 0) {
					close(fd);
					fclose(log);
					exit(1);
				}
				continue;
			}
			retry = RETRYCNT;
			d++;
		}
	}

	close(fd);
}

char *
get_response()
{
	static char buf[80];
	char *p = buf;
	int cnt;
	struct timeval t;
	fd_set ready;
	int fdlimit = 64;

	FD_ZERO(&ready);
	FD_SET(fd, &ready);

	t.tv_sec = 60;
	t.tv_usec = 0;

	if(select(fdlimit, &ready, NULL, NULL, &t) == 0)
		return NULL;

	if(FD_ISSET(fd, &ready)) {
		while(TRUE) {
			read(fd, p, 1);
			if(*p == '\n') {
				*p = 0;
				return buf;
			}
			p++;
		}
	}
	
	return NULL;
}

ignore(rsp, s, len, value)
char *rsp, *s;
int len, *value;
{
	return OK;
}

dummy(rsp, len, value)
char *rsp;
int len, *value;
{
	return OK;
}

rd_time(rsp, s, len, value)
char *rsp, *s;
int len, *value;
{
	if(index(s, ':') == NULL)
		return ERROR;

	strcpy(stime, s);
	return OK;
}

rd_date(rsp, s, len, value)
char *rsp, *s;
int len, *value;
{
	if(index(s, '/') == NULL)
		return ERROR;

	strcpy(sdate, s);
	return OK;
}

single_param(rsp, s, len, value)
char *rsp, *s;
int len, *value;
{
	if(strncmp(rsp, s, len))
		return ERROR;

	s += len;
	sscanf(s, "%d", value);
	return OK;
}

hi_low_param(rsp, s, len, n)
char *rsp, *s;
int len;
struct Hi_Low *n;
{
	if(strncmp(rsp, s, len))
		return ERROR;

	s += len;
	sscanf(s, "%d", &(n->value));
	s = (char*)index(s, ' ') + 1;
	strcpy(n->t_d, s);
	s = (char *)rindex(n->t_d, ':');
	*s = 0;
	return OK;
}

wind_param(rsp, s, len, value)
char *rsp, *s;
int len;
struct WindStruct *value;
{
	if(strncmp(rsp, s, len))
		return ERROR;

	s += len;
	sscanf(s, "%d", &(value->speed));
	s+=4;
	sscanf(s, "%d", &(value->direction));
	return OK;
}

wind_hi_low_param(rsp, s, len, n)
char *rsp, *s;
int len;
struct Wind_Hi_Low *n;
{
	if(strncmp(rsp, s, len))
		return ERROR;

	s += len;

	sscanf(s, "%d", &(n->v.speed));
	s+=4;
	sscanf(s, "%d", &(n->v.direction));

	s = (char*)index(s, ' ') + 1;
	strcpy(n->t_d, s);
	s = (char *)rindex(n->t_d, ':');
	*s = 0;
	return OK;
}

display_weather()
{
	printf("  OutTemp => %6d %s   %6d   %6d %s   %d\n",
		OutdoorTempLow.value, OutdoorTempLow.t_d, OutdoorTemp,
		OutdoorTempHigh.value, OutdoorTempHigh.t_d, OutdoorTempRate);
	printf("   InTemp => %6d %s   %6d   %6d %s   %d\n",
		IndoorTempLow.value, IndoorTempLow.t_d, IndoorTemp,
		IndoorTempHigh.value, IndoorTempHigh.t_d, IndoorTempRate);
	printf(" Humidity => %6d %s   %6d   %6d %s   %d\n",
		HumidityLow.value, HumidityLow.t_d, Humidity,
		HumidityHigh.value, HumidityHigh.t_d, HumidityRate);
	printf("Barometer => %6d %s   %6d   %6d %s   %d\n",
		BarometerLow.value, BarometerLow.t_d, Barometer,
		BarometerHigh.value, BarometerHigh.t_d, BarometerRate);

	printf("\n     Rain => %6d   %d\n", Rain, RainRate);

	printf("\n     Wind => %2d@%3d %s   %2d@%3d   %2d@%3d %s   %d\n",
		WindLow.v.speed, WindLow.v.direction, WindLow.t_d, 
		Wind.speed, Wind.direction,
		WindHigh.v.speed, WindHigh.v.direction, WindHigh.t_d, WindChill);
	printf("     Gust => %2d@%3d %s   %2d@%3d   %2d@%3d %s   %d\n",
		GustLow.v.speed, GustLow.v.direction, GustLow.t_d, 
		Gust.speed, Gust.direction,
		GustHigh.v.speed, GustHigh.v.direction, GustHigh.t_d, GustChill);
}

struct compass_points {
	int deg;
	char *dir;
} cp[17] = {
	{ 348, "N  " },
	{ 325, "NNW" },
	{ 303, "NW " },
	{ 280, "WNW" },
	{ 258, "W  " },
	{ 235, "WSW" },
	{ 213, "SW " },
	{ 190, "SSW" },
	{ 168, "S  " },
	{ 145, "SSE" },
	{ 123, "SE " },
	{ 100, "ESE" },
	{ 78, "E " },
	{ 55, "ENE" },
	{ 33, "NE " },
	{ 10, "ENE" },
	{ 0, "N  " }};

char *
compass(deg)
int deg;
{
	int i = 0;
	while(deg < cp[i].deg) i++;
	return cp[i].dir;
}

write_weather(indoor_also)
int indoor_also;
{
	FILE *fp;
	char *filename = indoor_also ? WxIndoor : WxOutdoor;

	if((fp = fopen(filename, "w")) == NULL)
		exit(1);

	fprintf(fp, "Weather Data as of %s %s\n", sdate, stime);
	fprintf(fp, "Mt. Umunhum, El 3350' 5 miles south of San Jose\n\n");

	fprintf(fp, 
		"                   Current  Rate          LOW                   HIGH\n");

	fprintf(fp,
		"Temperature (degF)   %3d    %+3d      %3d @ %s     %3d @ %s\n",
		OutdoorTemp, OutdoorTempRate,
		OutdoorTempLow.value, OutdoorTempLow.t_d,
		OutdoorTempHigh.value, OutdoorTempHigh.t_d);

	fprintf(fp,
		"Humidity (%%)         %3d    %+3d      %3d @ %s     %3d @ %s\n",
		Humidity, HumidityRate,
		HumidityLow.value, HumidityLow.t_d, 
		HumidityHigh.value, HumidityHigh.t_d);

	fprintf(fp,
		"Barometer (in)      %5.2f  %+4.2f   %5.2f @ %s   %5.2f @ %s\n",
		(float)Barometer/100, (float)BarometerRate/100,
		(float)BarometerLow.value/100, BarometerLow.t_d, 
		(float)BarometerHigh.value/100, BarometerHigh.t_d);

	fprintf(fp, "Rain (in)           %5.2f  %5.2f\n",
		(float)Rain/100, (float)RainRate/100);
	fprintf(fp, 
"                                        HIGH AVG             HIGH GUST\n");

	fprintf(fp,
		"Wind (mph)         %3d %s         %d %s @ %s  %d %s @ %s\n\n",
		Wind.speed, compass(Wind.direction), 
		WindHigh.v.speed, compass(WindHigh.v.direction), WindHigh.t_d,
		GustHigh.v.speed, compass(GustHigh.v.direction), GustHigh.t_d);

	fprintf(fp, 
		"Wind Chill (degF)    %3d               Weather Information\n",
		WindChill);
	fprintf(fp, 
		"Gust Chill (degF)    %3d               Courtesy of WA6YCZ\n", 
		GustChill);

	if(indoor_also) {
		fprintf(fp, 
		"\n  Indoor           Current  Rate          LOW                   HIGH\n");

		fprintf(fp,
			"Temperature (degF)   %3d    %+3d      %3d @ %s     %3d @ %s\n",
			IndoorTemp, IndoorTempRate,
			IndoorTempLow.value, IndoorTempLow.t_d,
			IndoorTempHigh.value, IndoorTempHigh.t_d);

		fprintf(fp,
			"Humidity (%%)         %3d    %+3d      %3d @ %s     %3d @ %s\n",
			HumidityIn, HumidityInRate,
			HumidityInLow.value, HumidityInLow.t_d, 
			HumidityInHigh.value, HumidityInHigh.t_d);
	}
	fclose(fp);
}

write_blank_record(errstr)
char *errstr;
{
	wx_log(errstr);

	OutdoorTempLow.value = 0;
	OutdoorTemp = 0;
	OutdoorTempHigh.value = 0;

	HumidityLow.value = 0;
	Humidity = 0;
	HumidityHigh.value = 0;

	BarometerLow.value = 0;
	Barometer = 0;
	BarometerHigh.value = 0;

	WindLow.v.speed = 0;
	WindLow.v.direction = 0;
	Wind.speed = 0;
	Wind.direction = 0;
	WindHigh.v.speed = 0;
	WindHigh.v.direction = 0;
	write_weather_raw();
}

write_weather_raw()
{
	FILE *fp;
	struct weather_data wd;

	if((fp = fopen(WxData, "a")) == NULL)
		exit(1);

	wd.when = time(NULL);
	wd.temp[WxLOW] = OutdoorTempLow.value;
	wd.temp[WxCURRENT] = OutdoorTemp;
	wd.temp[WxHIGH] = OutdoorTempHigh.value;

	wd.humidity[WxLOW] = HumidityLow.value;
	wd.humidity[WxCURRENT] = Humidity;
	wd.humidity[WxHIGH] = HumidityHigh.value;

	wd.barometer[WxLOW] = BarometerLow.value;
	wd.barometer[WxCURRENT] = Barometer;
	wd.barometer[WxHIGH] = BarometerHigh.value;

	wd.wind[WxLOW].speed = WindLow.v.speed;
	wd.wind[WxLOW].direction = WindLow.v.direction;
	wd.wind[WxCURRENT].speed = Wind.speed;
	wd.wind[WxCURRENT].direction = Wind.direction;
	wd.wind[WxHIGH].speed = GustHigh.v.speed;
	wd.wind[WxHIGH].direction = GustHigh.v.direction;

	wd.rain = Rain;

	fwrite(&wd, sizeof(wd), 1, fp);
	fclose(fp);
}

age_weather()
{
	rename(WxIndoor, WxIndoorYest);
	rename(WxOutdoor, WxOutdoorYest);
}

wx_log(str)
char *str;
{
	long t = time(NULL);
	struct tm *dt = localtime(&t);

	if(log == NULL) {

		if((log = fopen(WxLog, "a")) == NULL)
			return;
		fprintf(log, "\n");
	}

	fprintf(log, "\n[%02d/%02d %02d:%02d] %s",
		dt->tm_mon, dt->tm_mday, dt->tm_hour, dt->tm_min, str);
}
