#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"

#include "user.h"
#include "tokens.h"
#include "message.h"
#include "vars.h"

#define UP		0
#define DOWN	1
#define MAXRING 100

#define NOTHING		0
#define LISTCALLS	1
#define READMSG		2

static struct input_buffers {
	struct input_buffers *last, *next;
	long time;
	char s[256];
	int cnt;
} buf[MAXRING], *bptr, *cptr;

static void encode_connect_method(char *s, int method);

static char
	*xlate_call(char *call);

static int
	find_usernum(char *suffix),
	find_usernum_ascii(char *s),
	find_usernum_decimal(char *s),
	user_unread(int what, int msgnum),
	user_lastlogin(void),
	user_number(int unread, int usernum, int what, int msgnum),
	connect_cnt(char *s),
	connect_who(char *s),
	connect_status(int who),
	bbs_stats(char *s),
	hello(void),
	nts_messages(char *s),
	fwd_stats(char *s),
	current_time(char *s),
	bbs_access(char *s),
	lastlogin_by_number(char *s),
	held_messages(char *s),
#if 0
	rmt_cmd(char *s),
	rmt_play_str(char *s),
#endif
	unread_by_number(char *s);

static struct commands {
	char *code;
	int len;
	int (*func)();
} cmds[30] = {
	{ "01",	2,	fwd_stats },
	{ "02",	2,	current_time },
	{ "03", 2,  nts_messages },
	{ "04*", 3,  connect_who },
	{ "04", 2,  connect_cnt },
	{ "05", 2,  held_messages },
	{ "0",	1,	bbs_stats },

	{ "1*", 2,  unread_by_number },
	{ "1",  1,  unread_by_number },

	{ "2*", 2,  lastlogin_by_number },
	{ "2",  1,  lastlogin_by_number },

	{ "3*",	2,	find_usernum_ascii },
	{ "3",	1,	find_usernum_decimal },

	{ "7",	1,	bbs_access },

	{ 0, 0, 0 }
};

static char *dow[] = {
	" SUNDAY", " MONDAY", " TUESDAY",
	" WEDNESDAY", " THURSDAY", " FRIDAY",
	" SATURDAY"
};

static char *moy[] = {
	" JANUARY", " FEBRUARY", " MARCH", " APRIL",
	" MAY", " JUNE", " JULY", " AUGUST",
	" SEPTEMBER"," OCTOBER", " NOVEMBER", " DECEMBER"
};

static char *dom[] = {
	"0"," 1ST"," 2ND"," 3RD"," 4TH"," 5TH"," 6TH",
	" 7TH"," 8TH"," 9TH"," 10TH"," 11TH"," 12TH",
	" 13TH"," 14TH"," 15TH"," 16TH"," 17TH"," 18TH",
	" 19TH"," 20TH"," 21ST"," 22ND"," 23RD"," 24TH",
	" 25TH"," 26TH"," 27TH"," 28TH"," 29TH",
	" 30TH"," 31ST"
};

static void
	encode_day_of_week(char *s, time_t t),
	encode_month(char *s, time_t t),
	encode_day(char *s, time_t t),
	encode_time(char *s, time_t t),
	encode_number(char *s, int n);

static char play_str[10000];

static char *
xlate_call(char *call)
{
	char *p;
	static char out[80];

	p = out;
	*p++ = ' ';

	while(*call)
		*p++ = *call++;
	*p = 0;
	return out;
}

static int
under_repair(void)
{
	strcat(play_str, "Function under repair");
	return OK;
}


static int
lastlogin_by_number(char *s)
{
	return user_number(FALSE, get_number(&s), 0, 0);
}

static int
unread_by_number(char *s)
{
	int num = get_number(&s);
	
	switch(*s++) {
	case '*':
		return user_number(TRUE, num, LISTCALLS, 0);
	case '#':
		return user_number(TRUE, num, READMSG, get_number(&s));
	}
	return user_number(TRUE, num, NOTHING, 0);
}

static int
user_number(int unread, int usernum, int what, int msgnum)
{
	char *s;
	userd_open();
	s = user_by_number(usernum);
	if(!strncmp(s, "NO,", 3)) {
		sprintf(play_str, "No user %d in system.", usernum);
		userd_close();
		return OK;
	}
	strcpy(usercall, s);
	userd_fetch(usercall);

	if(what != READMSG)
		strcat(play_str, xlate_call(usercall));

	if(unread)
		user_unread(what, msgnum);
	else
		user_lastlogin();
	userd_close();
	return OK;
}

/*ARGSUSED*/
static int
held_messages(char *s)
{
	struct list_criteria lc;	
	int cnt;

	bzero(&lc, sizeof(lc));
	lc.must_include_mask = MsgHeld;
	lc.include_mask = MsgHeld;
	lc.range_type = FALSE;
	cnt = build_partial_list(&lc);
	flush_message_list();

	sprintf(play_str, "%s Held message count is, %d", play_str, cnt);
	return OK;
}

/*ARGSUSED*/
static int
nts_messages(char *s)
{
	struct list_criteria lc;	
	int cnt;

	bzero(&lc, sizeof(lc));
	lc.must_include_mask = MsgNTS | MsgActive;
	lc.include_mask = MsgNTS | MsgActive;
	lc.range_type = FALSE;
	lc.pattern_type = 0;
	cnt = build_partial_list(&lc);
	flush_message_list();

	sprintf(play_str, "%s NTS message count is, %d", play_str, cnt);
	return OK;
}

static int
user_unread(int what, int msgnum)
{
	struct list_criteria lc;	
	int cnt;

	if(what == READMSG && (msgnum < 1 || msgnum > 10))
		return OK;

	bzero(&lc, sizeof(lc));
	lc.must_include_mask = MsgNotReadByMe | MsgMine | MsgActive;
	lc.include_mask = MsgNotReadByMe | MsgMine | MsgActive;
	lc.range_type = FALSE;
	cnt = build_partial_list(&lc);

	if(what == READMSG) {
		struct msg_dir_entry *m = MsgDirList;
		while(m) {
			if(m->visible) {
				if(--msgnum == 0)
					break;
			}
			NEXT(m);
		}

		if(m) {
			struct text_line *tl;

			if(m->flags & MsgSecure) {
				talk("Can not play secure messages");
				flush_message_list();
				return OK;
			}
			msg_ReadBody(m);
			tl = m->body;

			while(tl) {
				if(strncmp(tl->s, "R:", 2)) {
					int len = strlen(tl->s)/15;

							/* let's check for termination indications,
							 * A line with only three spaces is start of
							 * the signature. Or look for the numbers 73.
							 */
					char *p = (char *)index(tl->s, '7'); 
					if(!strcmp(tl->s, "   \n"))
						break;
					if(p++ != NULL) {
						if((*p == '3') && !isalnum(*(p+1)))
							break;
					}

					talk(tl->s);
					sleep(len);
				}
				NEXT(tl);
			}
		}
		flush_message_list();
		return OK;
	}

	flush_message_list();

	sprintf(play_str, "%s has, %d un red messages", play_str, cnt);

	if((what == LISTCALLS) && (cnt != 0)) {
		struct msg_dir_entry *m = MsgDirList;
		while(m) {
			if(m->visible) {
				if(m->flags & MsgSecure)
					sprintf(play_str, "%s, secure", play_str);

				if(m->size / 1000)
					sprintf(play_str, "%s,\n%ld K message from %s subject %s", 
						play_str, m->size/1000, xlate_call(m->from.name.str),
						m->sub);
				else
					sprintf(play_str, "%s, \nfrom %s subject %s", 
						play_str, xlate_call(m->from.name.str), m->sub);
			}
			NEXT(m);
		}
	}
	return OK;
}

static int
user_lastlogin()
{
	under_repair();
#if 0
	strcat(play_str, " last log in via");
	encode_connect_method(play_str, ud->method);
	strcat(play_str, " on");
	encode_day_of_week(play_str, ud->last_seen);
	encode_month(play_str, ud->last_seen);
	encode_day(play_str, ud->last_seen);
	strcat(play_str, " at");
	encode_time(play_str, ud->last_seen);
#endif
	return OK;
}

static void
encode_connect_method(char *s, int method)
{
	char *str = " OOPS";

	switch(method) {
	case 3:
	case 4:
		str = " PHONE"; break;
	case 0:
		str = " 2 METER"; break;
	case 1:
		str = " 2 20"; break;
	case 2:
		str = " 4 40"; break;
	case 5:
		str = " CONSOLE"; break;
	}

	strcat(s, str);
}

static void
encode_day_of_week(char *s, time_t t)
{
	struct tm *tm = localtime(&t);	
	strcat(s, dow[tm->tm_wday]);
}

static void
encode_month(char *s, time_t t)
{
	struct tm *tm = localtime(&t);	
	strcat(s, moy[tm->tm_mon]);
}

static void
encode_day(char *s, time_t t)
{
	struct tm *tm = localtime(&t);	
	strcat(s, dom[tm->tm_mday]);
}

static void
encode_time(char *s, time_t t)
{
	struct tm *tm = localtime(&t);	
	sprintf(s, "%s %d", s, tm->tm_hour);
	if(tm->tm_min < 10)
		strcat(s, " 0");
	sprintf(s, "%s %d", s, tm->tm_min);
}

static void
encode_number(char *s, int n)
{
	int thousand = n/1000;
	int hundred;

	if(thousand) {
		encode_number(s, thousand);
		strcat(s, "THOUSAND ");
		n -= (thousand * 1000);
	}

	hundred = n/100;
	if(hundred) {
		encode_number(s, hundred);
		strcat(s, "HUNDRED ");
		n -= (hundred * 100);
	}

	if(n < 21)
		sprintf(s, "%s%d ", s, n);
	else {
		int ten = (n/10) * 10;
		sprintf(s, "%s%d ", s, ten);
		n -= ten;
		if(n != 0)
			sprintf(s, "%s%d ", s, n);
	}
}

/*ARGSUSED*/
static int
bbs_access(char *s)
{
	strcat(play_str, "The N0ARY bbs can be accessed on either 144 dot 93, or");
	strcat(play_str, " 433 dot 37 mega hertz,\nit can also be accessed by");
	strcat(play_str, " phone modem at area code 4 0 8, 7 4 9, 1950");
	return OK;
}

/*ARGSUSED*/
static int
bbs_stats(char *s)
{
	under_repair();
#if 0
	sprintf(play_str,
		"BBS info, total users %d, home users %d, total messages %d",
		usr_count(), wp_home_count(Bbs_Call), msg_count());
	return OK;
#endif
	return OK;
}

static int
hello(void)
{
	sprintf(play_str, " %s BBS remote link", Bbs_Call);
	return OK;
}

/*ARGSUSED*/
static int
fwd_stats(char *s)
{
	struct list_criteria lc;	
	int cnt;

	strcpy(play_str, " BBS forwarding info,");

	bzero(&lc, sizeof(lc));
	lc.must_include_mask = MsgPending;
	lc.include_mask = MsgPending;
	lc.range_type = FALSE;
	cnt = build_partial_list(&lc);
	flush_message_list();

	sprintf(play_str, "%s pending message count is, %d. ", play_str, cnt);
	user_number(FALSE, 3, FALSE, 0);

	return OK;
}

/*ARGSUSED*/
static int
current_time(char *s)
{
	long t = time(NULL);

	encode_day_of_week(play_str, t);
	encode_month(play_str, t);
	encode_day(play_str, t);
	strcat(play_str, " at");
	encode_time(play_str, t);
	return OK;
}


/*ARGSUSED*/
static int
connect_cnt(char *s)
{
	return connect_status(FALSE);
}

/*ARGSUSED*/
static int
connect_who(char *s)
{
	return connect_status(TRUE);
}

static int
connect_status(int who)
{
	FILE *fp;
	char buf[20][80];
	int i, where, cnt = 0;
	int method[5];
	char calllist[5][256];

	for(i=0; i<5; i++)
		method[i] = 0;
	strcpy(calllist[0], " ON 2 METER, ");
	strcpy(calllist[1], " ON 2 20, ");
	strcpy(calllist[2], " ON 4 40, ");
	strcpy(calllist[3], " ON PHONE, ");
	strcpy(calllist[4], " ON CONSOLE, ");

	fp = popen("ps -ax | grep \" bbs \" | grep -v grep", "r");
	while(fgets(buf[cnt], 80, fp)) {
		if(cnt++ >= 20)
			break;
	}
	pclose(fp);

	for(i=0; i<cnt; i++) {
		char call[20];
		char *p = &buf[i][24];

		case_strcpy(call, get_call(&p), AllUpperCase);
		switch(*p) {
		case '1': where = 0; break;
		case '2': where = 1; break;
		case '4': where = 2; break;
		case 'P': where = 3; break;
		case 'C': where = 4; break;
		}

		method[where]++;
		if(who) {
			strcat(calllist[where], xlate_call(call));
		
			strcat(calllist[where], ". ");
		}
	}
	

	sprintf(play_str, "%s %d currently connected,", play_str, cnt);

	for(i=0; i<5; i++)
		if(method[i]) {
			if(!who)
				sprintf(play_str, "%s %d", play_str, method[i]);
			strcat(play_str, calllist[i]);
		}

	return OK;
}

static int
find_usernum_ascii(char *s)
{
	char tmp[3];
	char *p, suffix[4];
	int c;
	int cnt = 0;

	tmp[2] = 0;
	p = suffix;

	while(*s && cnt < 3) {
		tmp[0] = *s++;
		if(*s == 0)
			return OK;
		tmp[1] = *s++;

		sscanf(tmp, "%x", &c);
		*p++ = (char)c;
		*p = 0;
		cnt++;
	}

	return find_usernum(suffix);
}

static int
find_usernum_decimal(char *s)
{
	char tmp[3];
	char *p, suffix[4];
	int cnt = 0;

	tmp[2] = 0;
	p = suffix;

	while(*s && cnt < 3) {
		tmp[0] = *s++;
		if(*s == 0)
			return OK;
		tmp[1] = *s++;

		*p++ = (char)(0x40 + atoi(tmp));
		*p = 0;
		cnt++;
	}

	return find_usernum(suffix);
}

/*ARGSUSED*/
static int
find_usernum(char *suffix)
{
	under_repair();
#if 0
	int found = FALSE;
	int fd;

	if((fd = open(USRDIR, O_RDWR)) < 0)
		return OK;

	while(read(fd, &ud, SizeofUserDir) > 0) {
		char *p = ud.call;
		while(isalpha(*p) && *p)
			p++;
		if(*p++ == 0)
			continue;

		if(!strcmp(suffix, p)) {
			sprintf(play_str, "%s %s is user,  %d", 
				play_str, xlate_call(ud.call), ud.number);
			found = TRUE;
		}
	}
	close(fd);

	if(!found)
		sprintf(play_str, " no users match %s", xlate_call(suffix));
#endif
	return OK;
}

#if 0
static int /* Is this even used? */
rmt_cmd(char *str)
{
	int i = 0;

	play_str[0] = 0;

	if(*str == 0)
		hello();
	else {
		while(cmds[i].len) {
			if(!strncmp(str, cmds[i].code, cmds[i].len)) {
				cmds[i].func(&(str[cmds[i].len]));
				return rmt_play_str(play_str);
			}
			i++;
		}
	}
	return OK;
}

static int
rmt_play_str(char *s)
{
	talk(s);
	return OK;
}
#endif

int
remote_access(char *str)
{
	int i;

	ImSysop = TRUE;
	IGavePassword = TRUE;

	if(strchr(str, 'D') == NULL) {
		if(*str == 0)
			hello();
		else {
			i = 0;
			while(cmds[i].len) {
				if(!strncmp(str, cmds[i].code, cmds[i].len)) {
					cmds[i].func(&(str[cmds[i].len]));
					break;
				}
				i++;
			}
		}

		if(play_str[0])
			talk(play_str);
	}
	return OK;
}

void
display_buf(struct input_buffers *p)
{
	p->s[p->cnt] = 0;
	printf("-> %s\n", p->s);
}
