#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#if HAVE_REGCOMP
#include <regex.h>
#endif /* HAVE_REGCOMP */

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"
#include "function.h"
#include "vars.h"
#include "tokens.h"
#include "event.h"
#include "body.h"
#include "file.h"

static char keyword[20][10];
static int number[20];
static int numcnt, keycnt;
static int disp_all = FALSE;
static time_t time_last_built = 0;

static time_t ev_t0, ev_t1, ev_delta;

static int EventCnt = 0;
static struct event_entry
	*Event = NULL;

static int
event_cnt()
{
	int fd = open(Bbs_Event_Dir, O_RDONLY);
	struct event_entry event;
	int cnt = 0;

	if(fd < 0)
		return 0;

	while(read(fd, (char*)&event, SizeofEvent)) {
		if(event.time)
			cnt++;
	}
	close(fd);
	return cnt;
}

static int
ev_compare_time(const void *a, const void *b)
{
	const struct event_entry *i = a, *j = b;

	return(i->time - j->time);
}

static void
event_build_directory()
{
	struct event_entry ev;
	struct dirent *dp;
	DIR *dirp = opendir(Bbs_Event_Path);
	struct tm tm;
	FILE *fpw;

	unlink(Bbs_Event_Dir);
	if((fpw = fopen(Bbs_Event_Dir, "w")) == NULL) {
		PRINTF("Couldn't open EVENTDIR for writing\n");
		return;
	}

	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
		FILE *fp;
		char buf[1024];

		if(!isdigit(dp->d_name[0]))
			continue;

		sprintf(buf, "%s/%s", Bbs_Event_Path, dp->d_name);
		if((fp = fopen(buf, "r")) == NULL) {
			PRINTF("Problem reading EVENT directory, %s\n", buf);
			closedir(dirp);
			fclose(fpw);
			return;
		}

		bzero(&ev, SizeofEvent);
		bzero(&tm, sizeof(tm));

		fgets(buf, 1024, fp);	/* ===== */
		fgets(buf, 1024, fp);	/* Date: */
		strptime(&buf[14], "%h %e %T %Y", &tm);
#ifdef SUNOS
		ev.time = timelocal(&tm);
#else
		ev.time = mktime(&tm); /* was timelocal -- rwp */
#endif
		fgets(buf, 1024, fp);	/* Event: */
		ev.number = atoi(&buf[10]);
		fgets(buf, 1024, fp);	/* Title: */
		buf[strlen(buf)-1] = 0;
		strcpy(ev.title, &buf[10]);
		fgets(buf, 1024, fp);	/* Loc: */
		buf[strlen(buf)-1] = 0;
		strcpy(ev.location, &buf[10]);
		fgets(buf, 1024, fp);	/* key: */
		buf[strlen(buf)-1] = 0;
		strcpy(ev.keyword, &buf[10]);

		fwrite(&ev, SizeofEvent, 1, fpw);
	}
	closedir(dirp);
	fclose(fpw);
}

static struct event_entry *
event_build_list()
{
	int filetime;

	if((filetime = file_last_altered(Bbs_Event_Dir)) == ERROR) {
		PRINTF("Error on stat to EVENTDIR\n");
		return NULL;
	}

	if(time_last_built < filetime) {
		struct event_entry *ev;
		int cnt, fd;

		if(Event)
			free(Event);

		EventCnt = cnt = event_cnt();
		Event = ev = malloc_multi_struct(cnt, event_entry);

		if((fd = Open(Bbs_Event_Dir, O_RDONLY)) < 0) {
			PRINTF("Erorr opening %s\n", Bbs_Event_Dir);
			return NULL;
		}

		while(cnt) {
			read(fd, (char*)ev, SizeofEvent);
			if(ev->time) {
				ev++;
				cnt--;
			}
		}
		Close(fd);

		qsort(Event, EventCnt, SizeofEvent, ev_compare_time);
	}
	return Event;
}

static int
event_check(struct event_entry *ev)
{
	int disp = FALSE;
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */
	if(disp_all)
		return TRUE;

	if(keycnt) {
		int i;
		for(i=0; i<keycnt; i++) {
#if HAVE_REGCOMP
			if (regcomp(&preg, keyword[i], 0) != 0) {
				PRINTF("Regular expression problem\n");
				return FALSE;
			}
			ret = regexec(&preg, ev->keyword, 0, NULL, 0);
			regfree(&preg);
			if (ret == 0) {
				disp = TRUE;
				break;
			}
#else
			if(re_comp(keyword[i])) {
				PRINTF("Regular expression problem\n");
				return FALSE;
			}
			if(re_exec(ev->keyword) == 1) {
				disp = TRUE;
				break;
			}
#endif /* HAVE_REGCOMP */
		}
		if(disp == FALSE)
			return FALSE;
	}

	ev_t1 = ev_t0 + ev_delta;
	if(ev->time < ev_t0 || ev->time > ev_t1)
		return FALSE;

	return TRUE;
}

static void
event_list()
{
	struct event_entry *ev = event_build_list();
	int i;

	for(i=0; i<EventCnt; i++) {
		if(event_check(ev)) {
			char dbuf[40];
			struct tm *dt = localtime(&(ev->time));
			strftime(dbuf, sizeof(dbuf), "%Y-%m-%d %a %R", dt);
			PRINTF("%4d %s %s @ %s\n", ev->number, dbuf, ev->title, ev->location);
		}
		ev++;
	}
}

static void
event_add()
{
	char buf[1024];
	struct tm tm;
	struct event_entry event, evtmp;
	int result, fd;
	long loc;
	FILE *fp;

	event.number = 0;

	bzero(&tm, sizeof(tm));

	PRINTF("Enter event date (YYYY-MM-DD):\n");
	GETS(buf, 1023);
	strptime(buf, "%Y-%m-%d", &tm);

	PRINTF("Enter event time (hh:mm):\n");
	GETS(buf, 1023);
	strptime(buf, "%H:%M", &tm);
	tm.tm_sec = 0;
#ifdef SUNOS
	event.time = timelocal(&tm);
#else
	event.time = mktime(&tm); /* was timelocal -- rwp */
#endif

	if(event.time < Time(NULL)) {
		PRINTF("The specified event has already happened\n");
		return;
	}

	PRINTF("Title:\n");
	GETS(buf, 1023);
	buf[39] = 0;
	strcpy(event.title, buf);

	PRINTF("Location (City, ST):\n");
	GETS(buf, 1023);
	buf[39] = 0;
	strcpy(event.location, buf);

	PRINTF("Keywords (flea exams meeting etc):\n");
	GETS(buf, 1023);
	case_string(buf, AllUpperCase);
	buf[59] = 0;
	strcpy(event.keyword, buf);

	fd = Open(Bbs_Event_Dir, O_RDWR);
	loc = 0;

	while(read(fd, (char*)&evtmp, SizeofEvent)) {
		event.number = evtmp.number + 1;
		loc += SizeofEvent;
	}

	lseek(fd, loc, SEEK_SET);
	write(fd, (char*)&event, SizeofEvent);
	Close(fd);

	sprintf(buf, "%s/%05d", Bbs_Event_Path, event.number);
	if((fp = fopen(buf, "w")) == NULL) {
		PRINTF("Error opening event file for writing\n");
		return;
	}

	fprintf(fp,
"======================================================================\n");

	fprintf(fp, "    Date: %s", ctime(&event.time));
	fprintf(fp, " Event #: %d\n", event.number);
	fprintf(fp, "   Title: %s\n", event.title);
	fprintf(fp, "Location: %s\n", event.location);
	fprintf(fp, "Keywords: %s\n", event.keyword);

	fprintf(fp,
"----------------------------------------------------------------------\n");
		
	PRINTF("Enter event description (/AB /EX /NU /ED /KI /AD /?):\n");
	{
		struct text_line *tl = NULL;
		result = get_body(&tl, EVENT_BODY, NULL, NULL, NULL);
		while(tl) {
			fprintf(fp, "%s\n", tl->s);
			NEXT(tl);
		}
		textline_free(tl);
	}

	fclose(fp);
	
	if(result == mABORT) {
		fd = Open(Bbs_Event_Dir, O_RDWR);
		lseek(fd, loc, SEEK_SET);
		event.time = 0;
		write(fd, (char*)&event, SizeofEvent);
		Close(fd);
		unlink(buf);
		return;
	}
}

static int
event_delete_num(int num)
{
	struct event_entry ev;
	int fd = Open(Bbs_Event_Dir, O_RDWR);
	char fn[80];
	long loc = 0;

	sprintf(fn, "%s/%05d", Bbs_Event_Path, num);

	while(read(fd, (char*)&ev, SizeofEvent)) {
		if(ev.number == num) {
			ev.time = 0;
			lseek(fd, loc, SEEK_SET);
			write(fd, (char*)&ev, SizeofEvent);
			unlink(fn);
			break;
		}
		loc += SizeofEvent;
	}
	Close(fd);
	return OK;
}

static void
event_delete()
{
	int i;
	for(i=0; i<numcnt; i++) {
		struct event_entry *ev = event_build_list();

		while(ev) {
			if(ev->number == number[i]) {
				PRINTF("DELETE [%s @ %s] (y/N): ", ev->title, ev->location);
				if(get_yes_no(NO) == YES)
					event_delete_num(ev->number);
				break;
			}
			ev++;
		}

		if(ev == NULL)
			PRINTF("Event #%d not found\n", number[i]);
		time_last_built = 0;
	}
}

static void
event_date(int month, int year)
{
	time_t t = Time(NULL);
	struct tm *tm = localtime(&t);

	if(month) {
		tm->tm_mon = month-1;
		tm->tm_year = year;
		tm->tm_mday = 1;
		tm->tm_min = tm->tm_hour = 0;
#ifdef SUNOS
		ev_t0 = timelocal(tm);
#else
		ev_t0 = mktime(tm); /* was timelocal -- rwp */
#endif
	} else {
		ev_t0 = t;
		ev_delta = 2 * tMonth;
	}
}

static void
event_interval(int func)
{
	if(numcnt > 1) {
		PRINTF("Only expected a single number in the command\n");
		PRINTF("Ignoring all additional numbers\n");
	}

	switch(func) {
	case DAYS:
		ev_delta = number[0] * tDay;
		break;
	case WEEKS:
		ev_delta = number[0] * tWeek;
		break;
	case MONTHS:
		ev_delta = number[0] * tMonth;
		break;
	case YEARS:
		ev_delta = number[0] * tYear;
		break;
	}
	numcnt = 0;
}

static void
event_disp()
{
	int i;

	for(i=0; i<numcnt; i++) {
		FILE *fp;
		char fn[80];
		char buf[1024];

		sprintf(fn, "%s/%05d", Bbs_Event_Path, number[i]);
		if((fp = fopen(fn, "r")) == NULL) {
			PRINTF("Event #%d not found\n", number[i]);
			continue;
		}

		while(fgets(buf, 1024, fp))
			PRINTF("%s", buf);
		PRINTF("\n");
		fclose(fp);
	}
}

int
event_t(void)
{
	struct TOKEN *t = TokenList;
	int func = 0;
	int year, month;

	event_date(0,0);

	numcnt = keycnt = 0;
	disp_all = FALSE;

	NEXT(t);
	if(t->token == END) {
		event_list();
		return OK;
	}

	while(t->token != END) {
		switch(t->token) {
		case AGE:
			event_aging();
			return OK;

		case COMPRESS:
			event_build_directory();
			return OK;

		case ALL:
			disp_all = TRUE;
			break;

		case ADD:
		case DELETE:
			if(func) return bad_cmd(-1, t->location);
			func = t->token;
			break;

		case DAYS:
		case MONTHS:
		case WEEKS:
		case YEARS:
			if(func) return bad_cmd(-1, t->location);
			func = t->token;
			event_interval(func);
			break;

		case STRING:
			if(sscanf(t->lexem, "%d/%d", &month, &year) == 2) 
				if(month >= 0 && month <= 12 && year < 100) {
					event_date(month, year);
					break;
				}
		case WORD:
			if(keycnt < 20) {
				t->lexem[9] = 0;
				case_string(t->lexem, AllUpperCase);
				strcpy(keyword[keycnt++], t->lexem);
			}
			break;	

		case NUMBER:
			if(numcnt < 20)
				number[numcnt++] = t->value;
			break;

		case NOPROMPT:
			no_prompt = TRUE;
			break;
		}
		NEXT(t);
	}

	switch(func) {
	case ADD:
		event_add();
		break;
	case DELETE:
		event_delete();
		break;

	default:
		if(numcnt)
			event_disp();
		else
			event_list();
	}

	return OK;
}

int
event_aging()
{
	int i;
	time_t t = Time(NULL) - tMonth;
	struct event_entry *ev = event_build_list();

	for(i=0; i<EventCnt; i++) {
		if(ev->time < t) {
			char fn[80];
			sprintf(fn, "%s/%05d", Bbs_Event_Path, ev->number);
			unlink(fn);
		}
		ev++;
	}

	event_build_directory();
	return OK;
}
