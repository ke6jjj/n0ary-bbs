#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "vars.h"

#define YESTERDAY		0
#define WEEKDAYS		1
#define WEEKENDS		2

#define DOTS			FALSE

#define DAYSperMONTH	(28)
#define SECSperHOUR		(60*60)
#define SECSperDAY		(SECSperHOUR*24)
#define SECSperWEEK		(SECSperDAY * 7)
#define SECSperMONTH	(SECSperWEEK * 28)
#define HOURSperDAY		(24)
#define HOURSperWEEK	(HOURSperDAY * 7)
#define HOURSperMONTH	(HOURSperDAY * DAYSperMONTH)

static char *port_name[6] = {
	"144.93 port",
	"223.62 port",
	"433.37 port",
	"Phone Port 749-1950",
	"Phone Port 749-0605",
	"Console (Gateway) Port" };

char string[256];
char yesterday[10];

struct hourly_bins {
	long used;
	int dow;
	long t0, t1;
	int cnt[6];
	long time[6];
} bins[HOURSperMONTH];

struct graphs {
	int cnt;
	long maxtime[6];
	long maxcnt[6];
	struct {
		long time[6];
		long cnt[6];
	} bins[HOURSperDAY];
} graph[3];

int use_stdout = FALSE;
FILE *output;

main(int argc, char **argv)
{
	int delay = 0;

	if(argc == 2) {
		use_stdout = TRUE;
		delay = atoi(argv[1]);
		printf("Delay by %d days\n", delay);
	}

	hist_read_database(delay);
	hist_condense();
	hist_port_time(0);
#if 1
	hist_port_time(1);
	hist_port_time(2);
	hist_port_time(3);
	hist_port_time(4);
	hist_port_time(5);
#endif

#if 0
	hist_connects();
	hist_time();
	hist_port_time(2);
	hist_port_time(4);
#endif
}

hist_read_database(int delay)
{
	long t;
	struct tm *tm;
	char buf[256];
	int i;
	int start = 0;
	FILE *fp;

		/* clear our hourly sturctures */

	bzero(&bins[0], sizeof(struct hourly_bins)*28*24);

		/* determine time value for midnight, last night */

	t = Time(NULL);
	tm = localtime(&t);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
#ifdef SUNOS
	t = timelocal(tm) -1;
#else
	t = mktime(tm) - 1;	/* was timelocal -- rwp */
#endif

	if(delay) {
		t -= delay*SECSperDAY;
	}

		/* set termination entry then scan list setting ranges */

	tm = localtime(&t);
	sprintf(yesterday, "%2d/%02d/%02d", tm->tm_mon+1, tm->tm_mday, tm->tm_year);
	bins[HOURSperMONTH-1].t1 = t;
	bins[HOURSperMONTH-1].dow = tm->tm_wday;

	for(i=HOURSperMONTH-1; i>=0; i--) {
		bins[i].t0 = bins[i].t1 - (SECSperHOUR - 1);
		if(i)
			bins[i-1].t1 = bins[i].t0 - 1;
	}


	for(i=HOURSperMONTH-1; i>0; i--) {
		int j, dow = bins[i].dow;
		for(j=0; j<23; j++) {
			bins[i-1].dow = bins[i].dow;
			i--;
		}
		bins[i-1].dow = bins[i].dow - 1;
		if(bins[i-1].dow < 0)
			bins[i-1].dow = 6;
	}

		/* begin reading in database */

	if((fp = fopen(HISTFILE, "r")) == NULL)
		return ERROR;

	while(fgets(buf, 256, fp)) {
		long t0, t1;
		long length;
		int connect, delta;

		sscanf(buf, "%d %d %d", &t0, &t1, &connect);
		delta = t1 - t0;

				/* take early exit if connect is not within the last
				 * 28 days.
				 */

		if((t1 < bins[0].t0) || (t0 > bins[HOURSperMONTH-1].t1))
			continue;

				/* find bin where connect is to start */

		for(i=start; i<HOURSperMONTH; i++) {
			if(t0 < bins[i].t1)
				break;
		}

		start = i-3;
		bins[i].used = TRUE;

				/* check to see if connect is within a single hour */

		if(t1 <= bins[i].t1) {
			bins[i].time[connect] += delta;
			bins[i].cnt[connect]++;
		} else {

				/* if not, we need to span hours */

			int remainder = bins[i].t1 - t0;

			bins[i].time[connect] += remainder;
			bins[i].cnt[connect]++;
			delta -= remainder;

			for(i++; i<HOURSperMONTH; i++) {
				bins[i].cnt[connect]++;
				if(delta < SECSperHOUR) {
					bins[i].time[connect] += delta;
					break;
				}
				delta -= SECSperHOUR;
				bins[i].time[connect] += SECSperHOUR;
			}
		}
	}
	fclose(fp);

#if 0
	for(i=HOURSperMONTH-HOURSperDAY, t=0; i<HOURSperMONTH; i++, t++) {
		printf("%2d> %3d %2d:%02d %3d %2d:%02d %3d %2d:%02d\n", t,
			bins[i].cnt[0], bins[i].time[0]/60, bins[i].time[0]%60,
			bins[i].cnt[1], bins[i].time[1]/60, bins[i].time[1]%60,
			bins[i].cnt[2], bins[i].time[2]/60, bins[i].time[2]%60);
	}
	getchar();
#endif
}

hist_condense(void)
{
	int i, j, k;
	int hour;
	int cnt; 

	for(i=0; i<3; i++) {
		graph[i].cnt = 0;
		for(j=0; j<6; j++)
			graph[i].maxtime[j] = graph[i].maxcnt[j] = 0;
	}

		/* load yesterdays info */

	graph[0].cnt = 1;
	hour = 0;
	for(i=HOURSperMONTH - HOURSperDAY; i<HOURSperMONTH; i++) {
		for(j=0; j<6; j++) {
			graph[0].bins[hour].time[j] = bins[i].time[j];
			graph[0].bins[hour].cnt[j] = bins[i].cnt[j];

			if(graph[0].bins[hour].time[j] > graph[0].maxtime[j])
				graph[0].maxtime[j] = graph[0].bins[hour].time[j];
			if(graph[0].bins[hour].cnt[j] > graph[0].maxcnt[j])
				graph[0].maxcnt[j] = graph[0].bins[hour].cnt[j];

			graph[1].bins[hour].cnt[j] = 0;
			graph[1].bins[hour].time[j] = 0;
			graph[2].bins[hour].cnt[j] = 0;
			graph[2].bins[hour].time[j] = 0;
		}
		hour++;
	}

		/* load history and keep a running count for average calc,
		 * first find first filled bin.
		 */

	for(i=0; i<DAYSperMONTH; i++)
		if(bins[i*HOURSperDAY].used)
			break;


	for(; i<DAYSperMONTH; i++) {
		int when = WEEKDAYS;
		if(bins[i*HOURSperDAY].dow == 0 || bins[i*HOURSperDAY].dow == 6)
			when = WEEKENDS;

		graph[when].cnt++;

		for(hour=0; hour<HOURSperDAY; hour++) {
			int hr = hour + (i*HOURSperDAY);
			for(j=0; j<6; j++) {
				graph[when].bins[hour].time[j] += bins[hr].time[j];
				graph[when].bins[hour].cnt[j] += bins[hr].cnt[j];
			}
		}
	}

#if 0
	printf("graph, prior to averaging\n");
	for(hour=0; hour<HOURSperDAY; hour++) {
		printf("%2d> %3d %3d:%02d  %3d %3d:%02d  %3d %3d:%02d\n", hour,
			graph[0].bins[hour].cnt[0],
			graph[0].bins[hour].time[0]/60, graph[0].bins[hour].time[0]%60,
			graph[1].bins[hour].cnt[0],
			graph[1].bins[hour].time[0]/60, graph[1].bins[hour].time[0]%60,
			graph[2].bins[hour].cnt[0],
			graph[2].bins[hour].time[0]/60, graph[2].bins[hour].time[0]%60);
	}
	printf("        %d           %d          %d\n",
		graph[0].cnt, graph[1].cnt, graph[2].cnt);
	getchar();
#endif

		/* do the averages */

	for(hour=0; hour<HOURSperDAY; hour++) {
		for(i=WEEKDAYS; i<=WEEKENDS; i++) {
			for(j=0; j<6; j++) {
				if(graph[i].cnt) {
					graph[i].bins[hour].time[j] /= graph[i].cnt;
					graph[i].bins[hour].cnt[j] /= graph[i].cnt;
				}

				if(graph[i].bins[hour].time[j] > graph[i].maxtime[j])
					graph[i].maxtime[j] = graph[i].bins[hour].time[j];
				if(graph[i].bins[hour].cnt[j] > graph[i].maxcnt[j])
					graph[i].maxcnt[j] = graph[i].bins[hour].cnt[j];
			}
		}
	}

#if 0
	printf("graph, after to averaging\n");
	for(hour=0; hour<HOURSperDAY; hour++) {
		printf("%2d> %3d %3d:%02d  %3d %3d:%02d  %3d %3d:%02d\n", hour,
			graph[0].bins[hour].cnt[0],
			graph[0].bins[hour].time[0]/60, graph[0].bins[hour].time[0]%60,
			graph[1].bins[hour].cnt[0],
			graph[1].bins[hour].time[0]/60, graph[1].bins[hour].time[0]%60,
			graph[2].bins[hour].cnt[0],
			graph[2].bins[hour].time[0]/60, graph[2].bins[hour].time[0]%60);
	}
	printf("\n    %3d %3d:%02d  %3d %3d:%02d  %3d %3d:%02d\n",
		graph[0].cnt, graph[0].maxtime[0]/60, graph[0].maxtime[0]%60,
		graph[1].cnt, graph[1].maxtime[0]/60, graph[1].maxtime[0]%60,
		graph[2].cnt, graph[2].maxtime[0]/60, graph[2].maxtime[0]%60);
	getchar();
#endif
}

#define RESOLUTION		300
#define LINESperHOUR	12

hist_port_time(int port)
{
	int i, cnt, mcnt[3];
	char fname[80];

#if DOTS
	sprintf(fname, "%s/time.%d", HISTDIR, port);
#else
	sprintf(fname, "%s/time%d", HISTDIR, port);
#endif
	if(use_stdout)
		output = stdout;
	else 
		if((output = fopen(fname, "w")) == NULL)
			exit();
	
	cnt = graph[YESTERDAY].maxtime[port];
	if(graph[WEEKDAYS].maxtime[port] > cnt)
		cnt = graph[WEEKDAYS].maxtime[port];
	if(graph[WEEKENDS].maxtime[port] > cnt)
		cnt = graph[WEEKENDS].maxtime[port];

	cnt /= RESOLUTION;
	if(cnt <= LINESperHOUR) {
		cnt = LINESperHOUR+1;
	}
	mcnt[YESTERDAY] = graph[YESTERDAY].maxtime[port]/RESOLUTION;
	mcnt[WEEKDAYS] = graph[WEEKDAYS].maxtime[port]/RESOLUTION;
	mcnt[WEEKENDS] = graph[WEEKENDS].maxtime[port]/RESOLUTION;

	initstring();
	putstring(0, " Connect time per hour ");
	putstring(23, port_name[port]);
	printstring();

	initstring();
	{
		char buf[256];
		sprintf(buf, 
#if DOTS
"       %s          |      Weekday Average     |     Weekend Average",
#else
"       %s          :      Weekday Average     :     Weekend Average",
#endif
		yesterday);
		putstring(0, buf);
	}

#if DOTS
	while(cnt>1) {
#else
	while(cnt) {
#endif
		int loc = 0;

		if((cnt) % LINESperHOUR == 0) {
#if DOTS
	putstring(0, "-------------------------:--------------------------:-------------------------");
#else
	putstring(0, "-------------------------|--------------------------|-------------------------");
#endif
		} else {
#if DOTS
			putstring(25, "|"); putstring(52, "|");
#else
			putstring(25, ":"); putstring(52, ":");
#endif
		}

		if(mcnt[YESTERDAY] == cnt) {
#if DOTS
			mcnt[YESTERDAY] -= 2;
#else
			mcnt[YESTERDAY]--;
#endif
			for(i=0; i<HOURSperDAY; i++) {
#if DOTS
				if(graph[YESTERDAY].bins[i].time[port]/RESOLUTION ==
					mcnt[YESTERDAY])
					putstring(loc+i, ".");

				if(graph[YESTERDAY].bins[i].time[port]/RESOLUTION >
					mcnt[YESTERDAY])
					putstring(loc+i, ":");
#else
				if(graph[YESTERDAY].bins[i].time[port]/RESOLUTION >
					mcnt[YESTERDAY])
					putstring(loc+i, "|");
#endif
			}
		}

#if 1
		loc = 27;
		if(mcnt[WEEKDAYS] == cnt) {
#if DOTS
			mcnt[WEEKDAYS] -= 2;
#else
			mcnt[WEEKDAYS]--;
#endif
			for(i=0; i<HOURSperDAY; i++) {
#if DOTS
				if(graph[WEEKDAYS].bins[i].time[port]/RESOLUTION ==
					mcnt[WEEKDAYS])
					putstring(loc+i, ".");

				if(graph[WEEKDAYS].bins[i].time[port]/RESOLUTION >
					mcnt[WEEKDAYS])
					putstring(loc+i, ":");
#else
				if(graph[WEEKDAYS].bins[i].time[port]/RESOLUTION >
					mcnt[WEEKDAYS])
					putstring(loc+i, "|");
#endif
			}
		}

		loc = 54;
		if(mcnt[WEEKENDS] == cnt) {
#if DOTS
			mcnt[WEEKENDS] -= 2;
#else
			mcnt[WEEKENDS]--;
#endif
			for(i=0; i<HOURSperDAY; i++) {
#if DOTS
				if(graph[WEEKENDS].bins[i].time[port]/RESOLUTION ==
					mcnt[WEEKENDS])
					putstring(loc+i, ".");

				if(graph[WEEKENDS].bins[i].time[port]/RESOLUTION >
					mcnt[WEEKENDS])
					putstring(loc+i, ":");
#else
				if(graph[WEEKENDS].bins[i].time[port]/RESOLUTION >
					mcnt[WEEKENDS])
					putstring(loc+i, "|");
#endif
			}
		}
#endif
		printstring();
		initstring();
#if DOTS
		cnt -= 2;
#else
		cnt--;
#endif
	}

	initstring();
	putstring(0,
"=========================+==========================+=========================");
	printstring();
	initstring();
	putstring(0, 
#if DOTS
"012345678901234567890123 | 012345678901234567890123 | 012345678901234567890123");
#else
"012345678901234567890123 : 012345678901234567890123 : 012345678901234567890123");
#endif
	printstring();

	if(use_stdout == FALSE)
		fclose(output);
}

#if 0
hist_port_connects(port)
int port;
{
	int i;
	int cnt; 

	mcnt = maxcnt[port];
	mtime = maxtime[port];
	mtime /= 600;

	cnt = mcnt;
	if(mtime > cnt) cnt = mtime;

	switch(port) {
	case 0:
		printf("\n  RF Port 144.93\n\n");
		break;
	case 1:
		printf("\n  RF Port 223.62\n\n");
		break;
	case 2:
		printf("\n  RF Port 433.37\n\n");
		break;
	case 3:
		printf("\n  Phone Port 749-0605\n\n");
		break;
	case 4:
		printf("\n  Phone Port 749-1950\n\n");
		break;
	case 5:
		printf("\n  Console (Gateway) Port\n\n");
		break;
	}

	while(cnt) {
		int loc = 0;
		initstring();
		if(mcnt == cnt) {
			mcnt--;
			for(i=0; i<24; i++) {
				if(bins[i].cnt[port] >= mcnt)
					putstring(loc+i, "*");
			}
		}

		loc = 27;
		if(mtime == cnt) {
			mtime--;
			for(i=0; i<24; i++) {
				if((bins[i].time[port]/600) >= mtime)
					putstring(loc+i, "*");
			}
		}

		putstring(25, "|");
		putstring(52, "|");
		printstring();
		cnt--;
	}
#if 1
	printf("-------------------------+--------------------------+-------------------------\n");
	printf("          11111111112222 |           11111111112222 |           11111111112222\n");
	printf("012345678901234567890123 | 012345678901234567890123 | 012345678901234567890123\n");
	printf("      Connects           |       Connect Time       |                  \n");
#else
	printf("-------------------------+--------------------------+-------------------------\n");
#endif

}

hist_connects()
{
	int i;
	mcnt = maxcnt[0];
	if(maxcnt[2] > mcnt) mcnt = maxcnt[2];
	if(maxcnt[4] > mcnt) mcnt = maxcnt[4];

	printf("\n  Number of connects by hour\n\n");
	while(mcnt--) {
		int loc = 0;
		initstring();
		for(i=0; i<24; i++) {
			if(bins[i].cnt[0] >= mcnt)
				putstring(loc+i, "*");
		}

		loc = 27;
		for(i=0; i<24; i++) {
			if(bins[i].cnt[2] >= mcnt)
				putstring(loc+i, "*");
		}

		loc = 54;
		for(i=0; i<24; i++) {
			if(bins[i].cnt[4] >= mcnt)
				putstring(loc+i, "*");
		}
		putstring(25, "|");
		putstring(52, "|");
		printstring();
	}
#if 1
	printf("-------------------------+--------------------------+-------------------------\n");
	printf("          11111111112222 |           11111111112222 |           11111111112222\n");
	printf("012345678901234567890123 | 012345678901234567890123 | 012345678901234567890123\n");
	printf("      144.93 port        |       433.37 port        |        Phone port\n");
#else
	printf("-------------------------+--------------------------+-------------------------\n");
#endif
}

hist_time()
{
	int i;
	printf("\n  Connect time by hour (* = 10 minutes)\n\n");

	mtime = maxtime[0];
	if(maxtime[2] > mtime) mtime = maxtime[2];
	if(maxtime[4] > mtime) mtime = maxtime[4];

	mtime /= 600;
	while(mtime--) {
		int loc = 0;
		initstring();
		for(i=0; i<24; i++) {
			if((bins[i].time[0]/600) >= mtime)
				putstring(loc+i, "*");
		}

		loc = 27;
		for(i=0; i<24; i++) {
			if((bins[i].time[2]/600) >= mtime)
				putstring(loc+i, "*");
		}

		loc = 54;
		for(i=0; i<24; i++) {
			if((bins[i].time[4]/600) >= mtime)
				putstring(loc+i, "*");
		}
		if((mtime+1) % 6 == 0) {
			putstring(25, "+"); putstring(52, "+");
		} else {
			putstring(25, "|"); putstring(52, "|");
		}
		printstring();
	}
	printf("-------------------------+--------------------------+-------------------------\n");
	printf("          11111111112222 |           11111111112222 |           11111111112222\n");
	printf("012345678901234567890123 | 012345678901234567890123 | 012345678901234567890123\n");
	printf("      144.93 port        |       433.37 port        |        Phone port\n");
}
#endif

initstring(void)
{
	int i;
	for(i=0; i<256; i++)
		string[i] = ' ';
}

putstring(int loc, char *s)
{
	int len = strlen(s);
	while(len--)
		string[loc++] = *s++;
}

printstring(void)
{
	string[78] = 0;
	fprintf(output, "%s\n", string);
}


