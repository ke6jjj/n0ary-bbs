#include <stdio.h>
#include <time.h>

#include "c_cmmn.h"
#include "tools.h"
#include "config.h"
#include "function.h"
#include "bbslib.h"
#include "tokens.h"
#include "vars.h"
#include "wx.h"

#define WxTEMP		1
#define WxHUMID		2
#define WxWIND		4
#define WxBARO		8
#define WxRAIN		0x10

#define WxGRAPH		1
#define WxDATA		2

#define WxOUT		0
#define WxIN		1
#define WxYEST		2

struct wx_data {
	long mo,day,hr,min;
	long show_range;
	long show_date;
	long good;
	int low, high, current;
};

struct wx_graph {
	int type;
	int low, high;
	int cnt;
	struct wx_data *data;
};

static void
	disp_wx_raw(int samples),
	print_baro_rule(void),
	print_rule(int low, int step, int high),
	print_graph_line(int type, int base, int step, int high, struct wx_data *wd),
	disp_wx_file(char *fname),
	disp_wx_graph(int type, int samples);


int
wx(void)
{
	struct TOKEN *t = TokenList;
	int file = 0;
	int param = 0;
	int func = 0;
	int samples = 16;

	while(t->token != END) {
		switch(t->token) {
		case BAROMETER:
			if(param)
				return bad_cmd(800, t->location);
			param = WxBARO;
			break;
		case TEMPERATURE:
			if(param)
				return bad_cmd(800, t->location);
			param = WxTEMP;
			break;
		case HUMIDITY:
			if(param)
				return bad_cmd(800, t->location);
			param = WxHUMID;
			break;
		case RAIN:
			if(param)
				return bad_cmd(800, t->location);
			param = WxRAIN;
			break;
		case WIND:
			if(param)
				return bad_cmd(800, t->location);
			param = WxWIND;
			break;

		case YESTERDAY:
			file |= WxYEST;
			break;
		case INDOOR:
			file |= WxIN;
			break;
	
		case SET:
			{
				char buf[80];
				PRINT("Enter password to authenticate yourself as GOD\n:");
				GETS(buf, 79);
			}
			return OK;

		case DATA:
			if(func)
				return bad_cmd(801, t->location);
			func = WxDATA;
			break;
		case GRAPH:
			if(func)
				return bad_cmd(801, t->location);
			func = WxGRAPH;
			break;

		case NUMBER:
			samples = t->value * 24;
			break;

		case LOG:
			disp_wx_file(Bbs_WxLog_File);
			return OK;
			
		}
		NEXT(t);
	}

	if(param) {
		if(file)
			return error(802);
		if(func == WxDATA)
			return error(803);
		disp_wx_graph(param, samples);
		return OK;
	}

	if(func == WxDATA) {
		if(file)
			return error(804);
		disp_wx_raw(samples);
		return OK;
	}

	switch(file) {
	case WxOUT:
		disp_wx_file(Bbs_WxOutdoor_File);
		return OK;
	case WxOUT | WxYEST:
		disp_wx_file(Bbs_WxOutdoorYest_File);
		return OK;
	case WxIN:
		disp_wx_file(Bbs_WxIndoor_File);
		return OK;
	case WxIN | WxYEST:
		disp_wx_file(Bbs_WxIndoorYest_File);
		return OK;
	}
	return OK;
}

static void
disp_wx_file(char *fname)
{
	FILE *fp;
	char buf[80];

	fp = fopen(fname, "r");
	if(fp == NULL) {
		PRINTF("Could not open %s file\n", fname);
		return;
	}

	while(fgets(buf, 80, fp))
		PRINT(buf);
	fclose(fp);
}

static void
disp_wx_graph(int type, int samples)
{
	struct weather_data wd;
	struct wx_graph graph;
	struct tm *dt;
	FILE *fp;
	int i, floor, step, ceiling;
	long rainbase;

	if(samples <= 0)
		return;

	if((fp = fopen(Bbs_WxData_File, "r")) == NULL) {
		PRINT("Couldn't open raw weather file\n");
		return;
	}

	fseek(fp, 0, 2);
	if(samples > (ftell(fp) / sizeof(wd)))
		rewind(fp);
	else
		fseek(fp, sizeof(wd) * samples * (-1), 2);

	bzero(&graph, sizeof(graph));
	graph.data = (struct wx_data *)calloc(samples, sizeof(struct wx_data));
	if(graph.data == NULL) {
		PRINT("disp_wx_graph: Memory allocation failure");
		return;
	}

	graph.type = type;
	graph.high = 0;
	graph.low = 9999;

	while(fread(&wd, sizeof(wd), 1, fp)) {
		dt = localtime(&wd.when);

		graph.data[graph.cnt].mo = dt->tm_mon+1;
		graph.data[graph.cnt].day = dt->tm_mday;
		graph.data[graph.cnt].hr = dt->tm_hour;
		graph.data[graph.cnt].min = dt->tm_min;

		if(graph.cnt) {
			if(graph.data[graph.cnt].day != graph.data[graph.cnt-1].day) {
				graph.data[graph.cnt-1].show_range = TRUE;
				graph.data[graph.cnt].show_date = TRUE;
			}
		} else
			graph.data[graph.cnt].show_date = TRUE;
			

		if(wd.barometer[WxCURRENT] == 0) {
			if(graph.cnt) {
				graph.data[graph.cnt].low = graph.data[graph.cnt-1].low;
				graph.data[graph.cnt].high = graph.data[graph.cnt-1].high;
			}
		} else {
			switch(type) {
			case WxTEMP:
				graph.data[graph.cnt].low = wd.temp[WxLOW];
				graph.data[graph.cnt].high = wd.temp[WxHIGH];
				graph.data[graph.cnt].current = wd.temp[WxCURRENT];
				break;
			case WxHUMID:
				graph.data[graph.cnt].low = wd.humidity[WxLOW];
				graph.data[graph.cnt].high = wd.humidity[WxHIGH];
				graph.data[graph.cnt].current = wd.humidity[WxCURRENT];
				break;
			case WxWIND:
				graph.data[graph.cnt].low = 0;
				graph.data[graph.cnt].high = wd.wind[WxHIGH].speed;
				graph.data[graph.cnt].current = wd.wind[WxCURRENT].speed;
				break;
			case WxBARO:
				graph.data[graph.cnt].low = wd.barometer[WxLOW];
				graph.data[graph.cnt].high = wd.barometer[WxHIGH];
				graph.data[graph.cnt].current = wd.barometer[WxCURRENT];
				break;
			case WxRAIN:
				graph.data[graph.cnt].high = 0;
				if(graph.cnt == 0) {
					rainbase = wd.rain;
					graph.data[graph.cnt].current = 0;
				} else
					graph.data[graph.cnt].current = (wd.rain - rainbase) / 10;
				break;
			}
			graph.data[graph.cnt].good = TRUE;

			if(graph.data[graph.cnt].low < graph.low)
				graph.low = graph.data[graph.cnt].low;
			if(graph.data[graph.cnt].high > graph.high)
				graph.high = graph.data[graph.cnt].high;
		}
		graph.cnt++;
	}
	graph.data[graph.cnt-1].show_range = TRUE;
	fclose(fp);

	switch(graph.type) {
	case WxTEMP:
		PRINT("\nTEMPERATURE (degF)\n");
		floor = 0; step = 2; ceiling = 110;
		break;
	case WxHUMID:
		PRINT("\nHUMIDITY (%)\n");
		floor = 0; step = 2; ceiling = 100;
		break;
	case WxWIND:
		PRINT("\nWIND (mph)\n");
		floor = 0; step = 2; ceiling = 110;
		break;
	case WxBARO:
		PRINT("\nBAROMETRIC PRESSURE (in)\n");
		floor = 2940; step = 2; ceiling = 3030;
		break;
	case WxRAIN:
		PRINT("\nRAIN (inches)\n");
		floor = 0; step = 1; ceiling = 50;
		break;
	}

	if(graph.type == WxBARO)
		print_baro_rule();
	else
		print_rule(floor, step, ceiling);

	for(i=0; i<graph.cnt; i++)
		print_graph_line(graph.type, floor, step, ceiling, &graph.data[i]);

	PRINT("\n");
	free(graph.data);
	return;
}

static void
print_graph_line(int type, int base, int step, int high, struct wx_data *wd)
{
	char buf[80];
	int loc, ceiling;

	if(wd->show_date)
		PRINTF("%02d/%02d %02d:%02d |", wd->mo, wd->day, wd->hr, wd->min);
	else
		PRINTF("      %02d:%02d |", wd->hr, wd->min);
	
	sprintf(buf, "                                                               ");
	ceiling = 0;

	if(wd->show_range && type != WxRAIN) {
		if(type != WxWIND) {
			if(wd->low < base)
				wd->low = base;
			if(wd->low > high)
				wd->low = high;
			loc = (wd->low - base) / step;
			buf[loc] = '<';
			if(loc > ceiling)
				ceiling = loc;
		}

		if(wd->high < base)
			wd->high = base;
		if(wd->high > high)
			wd->high = high;
		loc = (wd->high - base) / step;
		buf[loc] = '>';
		if(loc > ceiling)
			ceiling = loc;
	}

	if(wd->good) {
		if(wd->current < base)
			wd->current = base;
		if(wd->current > high)
			wd->current = high;
		loc = (wd->current - base) / step;
		buf[loc] = '*';
			if(loc > ceiling)
				ceiling = loc;
	}

	buf[ceiling+1] = 0;
	PRINTF("%s\n", buf);
}

static void
print_baro_rule(void)
{
	PRINT(
	"            29.  29.  29.  29.  29.  29.  30.  30.  30.  30.\n");
	PRINT(
	"             4    5    6    7    8    9    0    1    2    3\n");
	PRINT(
	"             0246802468024680246802468024680246802468024680\n");
	PRINT(
	"            +--------------------------|---------------------\n");
}

static void
print_rule(int low, int step, int high)
{
	char buf[4][120];
	int hundred = FALSE;
	int ten = FALSE;
	int i, j;

	for(i=0; i<75; i++)
		buf[0][i] = buf[1][i] = buf[2][i] = buf[3][i] = ' ';
	buf[0][i] = buf[1][i] = buf[2][i] = buf[3][i] = 0;

	for(i=13,j=low; j<=high; i++, j+=step) {
		if(j%10 == 0) {
			int num = j/10;
			if(num >= 10) {
				buf[0][i] = (num/10) + '0';
				hundred = TRUE;
				num -= 10;
			}
			if(j!=0) {
				buf[1][i] = num + '0';
				ten = TRUE;
			}
		}
		buf[2][i] = (j%10) + '0';
		buf[3][i] = '-';
	}
	buf[3][13] = '+';

	if(hundred)
		PRINTF("%s\n", buf[0]);
	if(ten)
		PRINTF("%s\n", buf[1]);
	PRINTF("%s\n", buf[2]);
	PRINTF("%s\n", buf[3]);
}

static void
disp_wx_raw(int samples)
{
	struct weather_data wd;
	struct tm *dt;
	FILE *fp;

	if(samples <= 0)
		return;

	if((fp = fopen(Bbs_WxData_File, "r")) == NULL) {
		PRINT("Couldn't open raw weather file\n");
		return;
	}

	fseek(fp, 0, 2);
	if(samples > (ftell(fp) / sizeof(wd)))
		rewind(fp);
	else
		fseek(fp, sizeof(wd) * samples * (-1), 2);

	PRINT(
"            -- Temp ---  -- Humid --  -- Barometer --  -- Wind ---\n");
	PRINT(
"            Min Cur Max  Min Cur Max   Min  Cur  Max    Avg  Gust   Rain\n");

	while(fread(&wd, sizeof(wd), 1, fp)) {
		dt = localtime(&wd.when);

		PRINTF("%2d/%02d %02d:%02d ",
			dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min);

		if(wd.barometer[WxCURRENT] == 0)
			PRINT("------------------------ Missed Sample ---------------------\n");
		else
			PRINTF( "%3d %3d %3d  %3d %3d %3d  %4d %4d %4d    %3d  %3d    %4d\n",
				wd.temp[WxLOW], wd.temp[WxCURRENT], wd.temp[WxHIGH],
				wd.humidity[WxLOW], wd.humidity[WxCURRENT], wd.humidity[WxHIGH],
				wd.barometer[WxLOW], wd.barometer[WxCURRENT], wd.barometer[WxHIGH],
				wd.wind[WxCURRENT].speed, wd.wind[WxHIGH].speed,
				wd.rain);
	}
	fclose(fp);
}
