#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "function.h"
#include "bbslib.h"
#include "user.h"
#include "vars.h"
#include "pending.h"
#include "tokens.h"
#include "tools.h"
#include "bbscommon.h"
#include "wp.h"

static void display_line(char *s);
static int force_more(void);

int 
	no_prompt,
	preempt_init_more = FALSE,
	more_cnt = 0;

static void
display_line(char *s)
{
	PRINTF("%s\n", s);
}

void
init_more(void)
{
	if(!preempt_init_more)
		more_cnt = Lines;
}

int
more(void)
{
	if(batch_mode || Lines == 0 || no_prompt)
		return OK;

	if(--more_cnt <= 0) {
		init_more();
		PRINT("==== More ... [CR] next page, [Q]uit, [S]croll, [!cmd] ====>");
		switch(get_quit_scroll_cmd(YES)) {
		case ERROR:
		case QUIT:
			return ERROR;
		case NO:
			no_prompt = TRUE;
			break;
		}
	}
	return OK;
}

int
conditional_more(void)
{
	if(batch_mode || Lines == 0 || no_prompt)
		return OK;

	return force_more();
}

static int
force_more(void)
{
	if(batch_mode)
		return OK;

	init_more();
	PRINT("==== More ... [CR] next page, [Q]uit, [S]croll, [!cmd] ====>");
	switch(get_quit_scroll_cmd(YES)) {
	case ERROR:
	case QUIT:
		return ERROR;
	case NO:
		no_prompt = TRUE;
		break;
	}
	return OK;
}

int
match_sysop_password(void)
{
	static int SysopPassword = FALSE;
	char buf[80];

	if(SysopPassword)
		return OK;

	if(!ImAllowedSysop) {
		PRINT("Sysop command, op rejected\n");
		return ERROR;
	}

	if(!port_secure(Via)) {
		PRINT("You are not on a secure channel, op rejected\n");
		return ERROR;
	}

	if(strcmp(Bbs_Sysop_Passwd, "")) {
		PRINT("Please enter sysop password: ");
		if(NeedsNewline)
			PRINT("\n");
		if (GETS(buf, 79) == NULL)
			return ERROR;
		if(strcmp(buf, Bbs_Sysop_Passwd))
			return ERROR;
	}
	PRINT("Approved ....\n");
	SysopPassword = TRUE;
	return OK;
}

int
find_pattern_match(char **str, char *pattern)
{
	int len = strlen(pattern);

	while(**str) {
		if(!strncmp(*str, pattern, len))
			return OK;
		(*str)++;
	}

	return ERROR;
}

char *
GETnSTR(char *str, int cnt, int caps)
{
	char buf[4096];
	if (GETS(buf, 4095) == NULL)
		return NULL;
	buf[cnt-1] = 0;
	case_string(buf, caps);
	strcpy(str, buf);
	return str;
}

char *
GETnSTRdef(char *str, int cnt, int caps, char *def)
{
	char defstr[256];
	strcpy(defstr, def);

	if (GETnSTR(str, cnt, caps) == NULL)
		return NULL;
	if(str[0] == ' ')
		*str = 0;
	else
		if(str[0] == 0)
			strcpy(str, defstr);
	return str;
}

void
case_string(char *p, int caps)
{
	switch(caps) {
	case CapFirst:
		case_string(p, AllLowerCase);
		NextChar(p);
		while(*p) {
			ToUpper(*p);
			NextSpace(p);
			NextChar(p);
		}
		break;

	case AllUpperCase:
		while(*p) {
			ToUpper(*p);
			p++;
		}
		break;

	case AllLowerCase:
		while(*p) {
			ToLower(*p);
			p++;
		}
		break;
	}
}

void
case_strcpy(char *to, char *from, int caps)
{
	strcpy(to, from);
	case_string(to, caps);
}

int
isNumber(char *s)
{
	while(*s) {
		if(!isdigit(*s++))
			return FALSE;
	}
	return TRUE;
}

int
isCall(char *s)
{
	int pre = 0, num = 0, suf =0, *part = &pre;
	int alpha = 0, digit = 0;
	char *p = s;

		/* first the call must contain both digits and letters but nothing
		 * else.
		 */

	while(*p) {
		if(isalpha(*p))
			alpha++;
		else if(isdigit(*p))
			digit++;
		else
			return FALSE;
		p++;
	}
	if(!digit || !alpha)
		return FALSE;

		/* check the wp to see if it is a valid callsign */
	if(wp_cmd(s) == OK)
		return TRUE;

		/* last resort, does it look like a valid call? */

	while(*s) {
		if(isalpha(*s))
			(*part)++;
		else {
			if(isdigit(*s)) {
				num++;
				part = &suf;
			} else
				return FALSE;
		}
		s++;
	}

	if(pre && pre < 3)
		if(num == 1)
			if(suf && suf < 4)
				return TRUE;


	return FALSE;
}

int
get_yes_no(int def)
{
	char buf[256];
	if(NeedsNewline)
		PRINT("\n");
	if (GETS(buf, 255) == NULL)
		return ERROR;
	switch(buf[0]) {
	case 'Y':
	case 'y':
		return YES;
	case 'N':
	case 'n':
		return NO;
	default:
		return def;
	}
}
	
int
get_quit_scroll_cmd(int def)
{
	char buf[256];
	if(NeedsNewline)
		PRINT("\n");
	if (GETS(buf, 255) == NULL)
		return ERROR;

	switch(buf[0]) {
	case 'S':
	case 's':
		return NO;
	case 'q':
	case 'Q':
		return QUIT;
	case '!':
		queue_pending_command(&buf[1]);
	}
	return def;
}

int
get_yes_no_quit(int def)
{
	char buf[256];
	if(NeedsNewline)
		PRINT("\n");
	if (GETS(buf, 255) == NULL)
		return ERROR;

	switch(buf[0]) {
	case 'y':
	case 'Y':
		return YES;
	case 'n':
	case 'N':
		return NO;
	case 'q':
	case 'Q':
		return QUIT;
	case '!':
		queue_pending_command(&buf[1]);
	}
	return def;
}

int
check_long_xfer_abort(int msg, int cnt)
{
	if(batch_mode || Lines || (cnt < 50)) 
		return OK;

	system_msg_number(msg, cnt);
	switch (get_yes_no(NO)) {
	case YES:
		system_msg(75);
	case ERROR:
		return ERROR;
	}

	return OK;
}

void
convert_dash2underscore(char *s)
{
	while(*s) {
		if(*s == '-')
			*s = '_';
		s++;
	}
}

void
trim_comment(char *s)
{
	char *p = (char *)index(s, ';');
	if(p == NULL)
		return;

	*p-- = 0;
	while(isspace(*p) && p != s)
		*p-- = 0;
}

