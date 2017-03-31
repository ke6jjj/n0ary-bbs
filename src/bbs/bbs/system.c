#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "system.h"
#include "function.h"
#include "user.h"
#include "version.h"

struct System *SystemList = NULL;

/*ARGSUSED*/
static void
system_parse_chat(int check, struct System *sys, int dir, char *p)
{
	struct System_chat **chat = &sys->chat;

	while(*chat)
		chat = &((*chat)->next);

	*chat = malloc_struct(System_chat);
	(*chat)->to = 60;

	if(*p == '"') {
		char *q;
		p++;
		if((q = (char*)index(p, '"')) != NULL) {
			*q = 0;

			/* In this form there is a possibility that we may have a
			 * a timeout value following the string. This is only valid
			 * when we have the quoted string.
			 */

			q++;
			NextChar(q);
			if(isdigit(*q))
			   	(*chat)->to = atoi(q);
		}
	}

	strcpy((*chat)->txt, p);
	(*chat)->dir = dir;
}

/*ARGSUSED*/
static void
system_parse_opt(int check, struct System *sys, char *p)
{
	char str[256];

	while(*p) {
		strcpy(str, get_string(&p));
		case_string(str, AllUpperCase);

		if(!strcmp(str, "NOHLOC"))
			sys->options |= optNOHLOC;
		if(!strcmp(str, "DUMBTNC"))
			sys->options |= optDUMBTNC;
		else if(!strcmp(str, "POLL"))
			sys->options |= optPOLL;
		else if(!strcmp(str, "MULTI"))
			sys->options |= optSMTPMUL;
		else if(!strcmp(str, "NOREVFWD"))
			sys->options &= ~optREVFWD;
		else if(!strcmp(str, "AGE")) {
			sys->age = get_number(&p);
			ToUpper(*p);
			switch(*p) {
			case 'H':
				sys->age *= tHour;
				break;
			case 'W':
				sys->age *= tWeek;
				break;
			case 'D':
			default:
				sys->age *= tDay;
				break;
			}
			NextSpace(p);
			NextChar(p);
		} else {
			char buf[256];
			sprintf(buf, "Illegal OPT in Systems file [%s]", str);
			problem_report(Bbs_Call, BBS_VERSION, sys->alias->alias, buf);
		}
	}
}

/*ARGSUSED*/
static void
system_parse_ax25(int check, struct System *sys, char *p)
{
	char str[256];
	int value;

	while(*p) {
		strcpy(str, get_string(&p));
		case_string(str, AllUpperCase);
		value = get_number(&p);

		if(value == 0)
			continue;

		if(!strcmp(str, "T1"))
			sys->ax25.t1 = value;
		else
		if(!strcmp(str, "T2"))
			sys->ax25.t2 = value;
		else
		if(!strcmp(str, "T3"))
			sys->ax25.t3 = value;
		else
		if(!strcmp(str, "MAXFRAME"))
			sys->ax25.maxframe = value;
		else
		if(!strcmp(str, "PACLEN"))
			sys->ax25.paclen = value;
		else
		if(!strcmp(str, "N2"))
			sys->ax25.n2 = value;
		else
		if(!strcmp(str, "PTHRESH"))
			sys->ax25.pthresh = value;
	}
}

/*ARGSUSED*/
static void
system_parse_when(int check, struct System *sys, char *p)
{
	time_t t = Time(NULL);
	struct tm *dt = localtime(&t);
	int now = (dt->tm_hour * 100) + dt->tm_min;
	int time_ok = TRUE;
	int dow_ok = FALSE;

	sys->t0 = sys->t1 = sys->dow = 0;
	sys->now = FALSE;

	NextChar(p);

	if(*p == 'N' || *p == 'n') {
		sys->now = FALSE;
		return;
	}

	switch(*p) {
	case '1': case '2': case '3': case '4': case '5': 
	case '6': case '7': case '8': case '9': case '0': 
		sys->t0 = get_number(&p);
		sys->t1 = get_number(&p);

		if((sys->t0 < 0 || sys->t0 > 2400) || 
		(sys->t1 < 0 || sys->t1 > 2400) ||
		(sys->t1 < sys->t0)) {
			PRINTF("Error in time range: %s\n", p);
			sys->now = FALSE;
			return;
		}

		if(sys->t0 >= now || now >= sys->t1)
			time_ok = FALSE;
		break;
	case '*':
		sys->t0 = 0;
		sys->t1 = 2359;
		p++;
		NextChar(p);
		break;

	default:
		PRINTF("Invalid WHEN date field\n");
		sys->now = FALSE;
		return;
	}

	if(*p == '*') {
		sys->now = time_ok;
		return;
	}

	while(isdigit(*p)) {
		switch(*p) {
		case '0': 
			sys->dow |= dowSUNDAY; break;
		case '1':
			sys->dow |= dowMONDAY; break;
		case '2':
			sys->dow |= dowTUESDAY; break;
		case '3':
			sys->dow |= dowWEDNESDAY; break;
		case '4':
			sys->dow |= dowTHURSDAY; break;
		case '5': 
			sys->dow |= dowFRIDAY; break;
		case '6': 
			sys->dow |= dowSATURDAY; break;

		default:
			PRINTF("Invalid WHEN dow field\n");
			sys->now = FALSE;
			return;
		}

		if(*p++ - '0' == dt->tm_wday)
			dow_ok = TRUE;

		if(dow_ok && time_ok)
			sys->now = TRUE;
	}
}

/*ARGSUSED*/
static void
system_parse_connect(int check, struct System *sys, char *p)
{
	strcpy(sys->connect, p);
}

/*ARGSUSED*/
static void
system_parse_aliases(int check, struct System *sys, char *p)
{
	struct System_alias **name = &sys->alias;

	do {
		*name = malloc_struct(System_alias);	
		strcpy((*name)->alias, get_string(&p));
		name = &((*name)->next);
	} while(*p != 0);
}

static struct System *
system_free_list(struct System *sys)
{
	struct System *tmp;

	while(sys) {
		while(sys->alias) {
			struct System_alias *alias = sys->alias;
			sys->alias = alias->next;
			free(alias);
		}
		while(sys->chat) {
			struct System_chat *chat = sys->chat;
			sys->chat = chat->next;
			free(chat);
		}
		if(sys->port != NULL)
			free(sys->port);

		if(sys->sid != NULL)
			free(sys->sid);

		if(sys->order != NULL)
			free(sys->order);

		tmp = sys;
		NEXT(sys);
		free(tmp);
	}
	return NULL;
}

/*ARGSUSED*/
static void
system_read_file(int check)
{
	char buf[256];
	struct System **sys = &SystemList;
	FILE *fp = fopen(Msgd_System_File, "r");

	if(SystemList != NULL)
		SystemList = system_free_list(SystemList);

	if(fp == NULL) {
		PRINTF("Couldn't open Msgd_System_File file\n");
		return;
	}

	while(fgets(buf, 256, fp)) {
		if(!isalpha(buf[0]))
			continue;

		*sys = malloc_struct(System);

		buf[strlen(buf) - 1] = 0;
		trim_comment(buf);
		case_string(buf, AllUpperCase);
		system_parse_aliases(check, *sys, buf);

		(*sys)->now = TRUE;
		(*sys)->t0 = 0;
		(*sys)->t1 = 2359;
		(*sys)->dow = dowEVERYDAY;
		(*sys)->options = optREVFWD;
		(*sys)->order = copy_string("*");

		while(fgets(buf, 256, fp)) {
			char *q, *p = buf;

			buf[strlen(buf) - 1] = 0;
			trim_comment(buf);

			NextChar(p);

			if(*p == 0)
				break;;

			q = p;
			NextSpace(q);
			NextChar(q);


			if(!strncmp(p, "OPT", 3))
				system_parse_opt(check, *sys, q);
			else
			if(!strncmp(p, "PORT", 4))
				(*sys)->port = copy_string(get_string(&q));
			else
			if(!strncmp(p, "CONNECT", 7))
				system_parse_connect(check, *sys, q);
			else
			if(!strncmp(p, "RECV", 4))
				system_parse_chat(check, *sys, chatRECV, q);
			else
			if(!strncmp(p, "SEND", 4))
				system_parse_chat(check, *sys, chatSEND, q);
			else
			if(!strncmp(p, "DELAY", 5))
				system_parse_chat(check, *sys, chatDELAY, q);
			else
			if(!strncmp(p, "WHEN", 4))
				system_parse_when(check, *sys, q);
			else
			if(!strncmp(p, "SID", 3))
				(*sys)->sid = copy_string(get_string(&q));
			else
			if(!strncmp(p, "AX25", 4))
				system_parse_ax25(check, *sys, q);
			else
			if(!strncmp(p, "ORDER", 5))
				(*sys)->order = copy_string(q);
		}
		sys = &(*sys)->next;
	}
}


int
system_chk(void)
{
	system_read_file(TRUE);
	return OK;
}

int
system_open(void)
{
	system_read_file(FALSE);
	return OK;
}

void
system_close(void)
{
	if(SystemList != NULL)
		SystemList = system_free_list(SystemList);
}

int
system_valid_alias(char *name)
{
	struct System *sys = SystemList;

	case_string(name, AllUpperCase);

	if(sys == NULL) {
		system_open();
		sys = SystemList;
	}

	while(sys) {
		struct System_alias *alias = sys->alias;
		while(alias) {
			if(!strcmp(alias->alias, name))
				return TRUE;
			NEXT(alias);
		}
		NEXT(sys);
	}
	return FALSE;
}

char *
system_disp_alias(char *name)
{
	struct System *sys = SystemList;
	static char buf[1024];
	int found = 0;
	char *p;

	strcpy(buf, "[ ");
	case_string(name, AllUpperCase);

	if(sys == NULL) {
		system_open();
		sys = SystemList;
	}

	while(sys) {
		struct System_alias *alias = sys->alias;
		p = alias->alias;
		NEXT(alias);

		while(alias) {
			if(!strcmp(alias->alias, name)) {
				found++;
				sprintf(buf, "%s%s ", buf, p);
				break;
			}
			NEXT(alias);
		}
		NEXT(sys);
	}
	strcat(buf, "]");

	if(found)
		return buf;
	return "";
}

int
system_my_alias(char *call, char *name)
{
	struct System *sys = SystemList;

	case_string(name, AllUpperCase);
	case_string(call, AllUpperCase);

	if(sys == NULL) {
		system_open();
		sys = SystemList;
	}

	while(sys) {
		struct System_alias *alias = sys->alias;
		if(!strcmp(alias->alias, call)) {
			while(alias) {
				if(!strcmp(alias->alias, name))
					return TRUE;
				NEXT(alias);
			}
			return FALSE;
		}
		NEXT(sys);
	}
	return FALSE;
	
}

char *
system_get_exp_sid(char *alias)
{
	static char sid[80];
	struct System *sys;
	int found = FALSE;

	system_open();
	sys = SystemList;
	while(sys) {
		if(!strcmp(sys->alias->alias, usercall)) {
			if(sys->sid != NULL) {
				found = TRUE;
				strcpy(sid, sys->sid);
				break;
			}
		}
		NEXT(sys);
	}
	system_close();

	if(found == TRUE)
		return sid;
	return NULL;
}
