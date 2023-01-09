#include <stdio.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "help.h"
#include "vars.h"
#include "server.h"
#include "history.h"
#include "wp.h"
#include "bbscommon.h"

static void dump_all_helps(void);

FILE *hlpmsg_fp = NULL;

static int 
	hlpmsg_value = 9876,
	hlpmsg_cnt = 0;

static long *hlpmsg_indx = NULL;

static char
	hlpmsg_string[80];

int
	disable_help = FALSE;

int
help(void)
{
	struct TOKEN *t = TokenList;

	switch(t->token) {
	case NUMBER:
		if(t->value < 0 || t->value > 3)
			PRINT("Help levels must be between 0 and 3\n");
		else
			user_set_value(uHELP, t->value);
		break;
	case END:
		system_msg(959);
		break;
	case ALL:
		dump_all_helps();
		break;

	default:
		while(t->token != END) {
			if(system_msg(t->value))
				return OK;
			PRINT("\n");
			NEXT(t);
		}
	}
	return OK;
}

static void
dump_all_helps(void)
{
	init_more();
	system_msg(900);
}

int
initialize_help_message(void)
{
	struct stat sbuf;
	FILE *fp;

	strcpy(hlpmsg_string, "DEFAULT STRING");

	if(stat(Bbs_HelpMsg_Index, &sbuf)) {
		error_log("initialize_help_message.stat(): %s", sys_errlist[errno]);
		return ERROR;
	}

	if((fp = fopen(Bbs_HelpMsg_Index, "r")) == NULL) {
		error_log("initialize_help_message.fopen(%s): %s",
			Bbs_HelpMsg_Index, sys_errlist[errno]);
		return ERROR;
	}

	hlpmsg_cnt = sbuf.st_size / sizeof(long);
	
	if((hlpmsg_indx = (long*)malloc(sbuf.st_size)) == NULL) {
		error_log("initialize_help_message.malloc(): %s", sys_errlist[errno]);
		fclose(fp);
		return ERROR;
	}

	if(!fread(hlpmsg_indx, sbuf.st_size, 1, fp)) {
		fclose(fp);
		free(hlpmsg_indx);
		hlpmsg_indx = NULL;
		error_log("initialize_help_message.fread(indx): %s",
			sys_errlist[errno]);
		return ERROR;
	}

	fclose(fp);

	if((hlpmsg_fp = fopen(Bbs_HelpMsg_Data, "r")) == NULL) {
		error_log("initialize_help_message.fopen(%s): %s",
			Bbs_HelpMsg_Data, sys_errlist[errno]);
		free(hlpmsg_indx);
		hlpmsg_indx = NULL;
		return ERROR;
	}

	return OK;	
}
	
void
close_down_help_messages(void)
{
	fclose(hlpmsg_fp);
	free(hlpmsg_indx);
	hlpmsg_indx = NULL;
}

static void
replace_macro(char **p, char c)
{
	char *q = "[UNKNOWN]";
	char buf[80];

	switch(c) {
#if 0
	case 'Q':
		q = BBS_FREQ;
		break;
#endif

	case 'B':
		q = Bbs_Call;
		break;

	case 'C':
		q = usercall;
		break;

	case 'M':
		sprintf(buf, "%d", active_message);
		q = buf;
		break;

	case 'N':
		sprintf(buf, "%d", hlpmsg_value);
		q = buf;
		break;

	case '!':
		sprintf(buf, "%d", cmd_number);
		q = buf;
		break;

	case 'U':
		sprintf(buf, "%ld", user_get_value(uNUMBER));
		q = buf;
		break;

	case 'S':
		q = hlpmsg_string;
		break;

	case 'H':
		q = wp_get_field(usercall, wHOME);
		break;

	case 'L':
		q = user_get_field(uLNAME);
		break;

	case 'F':
		q = wp_get_field(usercall, wFNAME);
		break;

	case '%':
		q = "%";
		break;
	}
		
	while(*q)
		*(*p)++ = *q++;
}

static char *
help_macro_exp(char *str)
{
	static char buf[256];
	char *p = buf;

	if(strrchr(str, '%') == NULL)
		return str;

	while(*str) {
		if(*str == '%') {
			str++;
			replace_macro(&p, *str++);
			continue;
		}
		*p++ = *str++;
	}

	*p = 0;
	return buf;
}


void
system_msg_number(int num, int value)
{
	hlpmsg_value = value;
	system_msg(num);
}

void
system_msg_string(int num, char *str)
{
	strcpy(hlpmsg_string, str);
	system_msg(num);
}

void
system_msg_numstr(int num, int value, char *str)
{
	hlpmsg_value = value;
	strcpy(hlpmsg_string, str);
	system_msg(num);
}

int
system_msg(int num)
{
	int mask = 1 << MyHelpLevel;
	char buf[256];

	if(InServer || PendingCmd || disable_help || num == -1)
		return OK;

	if(num > hlpmsg_cnt) {
		PRINTF("Supplied help message number %d is out of range, %d max\n",
			num, hlpmsg_cnt);
		return ERROR;
	}

	if(hlpmsg_indx[num] == 0)
		return OK;

	fseek(hlpmsg_fp, hlpmsg_indx[num], 0);
	
	init_more();
	while(fgets(buf, 256, hlpmsg_fp)) {
		int value = atoi(buf);
		char *p;

		if(buf[0] == '=')
			return OK;

		if(!NeedsNewline) {
			if((p = (char*)rindex(buf, '\n')) != NULL) {
				p--;
				switch(*p) {
				case ':':
				case '?':
					p++; *p = ' ';
				}
			}
		}

		if(value & mask) {
			if(buf[5]) {
				PRINT(help_macro_exp(&buf[5]));
			} else {
				PRINT("\n");
			}
		}
		if(more()) {
			return ERROR;
		}
	}

	PRINT("Error reading help message file\n");
	return ERROR;
}

int
bad_cmd(int errmsg, int location)
{
	PRINT(CmdLine);
	PRINT("\n");
	while(location--)
		PRINT(" ");
	PRINTF("^-- Syntax Error\n");
	if(ImBBS)
		exit_bbs();
	system_msg(errmsg);
	return ERROR;
}

int
bad_cmd_double(int errmsg, int loc1, int loc2)
{
	int loc = 0;

	while(loc++ != loc1)
		PRINT(" ");
	PRINT("^");
	while(loc++ != loc2)
		PRINT("-");

	PRINTF("^-- Syntax Error\n");
	system_msg(errmsg);
	return ERROR;
}

int
error(int errmsg)
{
	system_msg(errmsg);
	return ERROR;
}

int
error_number(int errmsg, int n)
{
	system_msg_number(errmsg, n);
	return ERROR;
}

int
error_string(int errmsg, char *str)
{
	system_msg_string(errmsg, str);
	return ERROR;
}


void
show_command_menus(int which)
{
	if(which & SystemMenu)
		system_msg(40);
	if(which & MessageMenu)
		system_msg(41);
	if(which & FileMenu)
		system_msg(42);
	if(which & UserMenu)
		system_msg(43);
	if(which & SysopMenu)
		system_msg(44);
	system_msg(45);
}
