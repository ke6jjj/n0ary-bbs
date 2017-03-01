#include <stdio.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"
#include "function.h"
#include "wp.h"
#include "tokens.h"  /* just to pickup DBG stuff */


static char disp_buf[85];

extern int
	debug_level;

void
usr_disp_gateallow(char *s)
{
	PRINTF("\t%s\n", s);
}

static void
usr_disp_ports(char *s)
{
	time_t t;
	struct tm *tm;
	char name[80];
	struct PortDefinition *pd;

	space_fill(disp_buf, 79);
	place_string(disp_buf, get_string(&s), 0);
	pd = port_find(get_string(&s));

	if(pd->show == FALSE)
		return;

	place_string(disp_buf, pd->name, 7);
	place_string(disp_buf, pd->alias, 15);
	place_string(disp_buf, get_string(&s), 25);
	place_number(disp_buf, get_number(&s), 31);

	t = (time_t)get_number(&s);
	if(t == 0)
		place_string(disp_buf, "Never", 43);
	else {
		tm = localtime(&t);
		strftime(name, 80, "%D @ %R", tm);
		place_string(disp_buf, name, 37);
	}

	t = (time_t)get_number(&s);
	if(t == 0)
		place_string(disp_buf, "Never", 63);
	else {
		tm = localtime(&t);
		strftime(name, 80, "%D @ %R", tm);
		place_string(disp_buf, name, 57);
	}

	PRINTF("%s\n", disp_buf);
}

char *
usr_disp_list_entry(struct IncludeList *list)
{
	struct IncludeList *t;
	static char buf[80];

	buf[0] = 0;
	t = list;
	while(t) {
		if(t->key == '>')
			sprintf(buf, "%s%s", buf, t->str);
		t = t->comp;
	}

	t = list;
	while(t) {
		if(t->key == '@')
			sprintf(buf, "%s%c%s", buf, t->key, t->str);
		t = t->comp;
	}

	t = list;
	while(t) {
		if(t->key == '<')
			sprintf(buf, "%s %c%s", buf, t->key, t->str);
		t = t->comp;
	}

	return buf;
}

void
usr_disp_list(struct IncludeList *list)
{
	while(list) {
		PRINTF("    %s\n", usr_disp_list_entry(list));
		NEXT(list);
	}
}

void
user_disp_account(char *call)
{
	int i = 0;
	char field[LenMAX];

	uppercase(call);
	if(user_focus(call) == ERROR) {
		PRINTF("%s is not a user of this system\n", call);
		return;
	}

	space_fill(disp_buf, 79);
	place_string(disp_buf, call, 9);
	place_string(disp_buf, "usernum:", 57);
	place_number(disp_buf, user_get_value(uNUMBER), 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "FName:", 2);
	place_string(disp_buf, wp_get_field(call, wFNAME), 9);
	place_string(disp_buf, "status:", 58);
	if(user_check_flag(uNONHAM))
		place_string(disp_buf, "NONHAM", 66);
	else
		if(user_check_flag(uAPPROVED))
			place_string(disp_buf, "APPROVED", 66);
		else
			place_string(disp_buf, "RESTRICTED", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "LNAme:", 2);
	place_string(disp_buf, user_get_field(uLNAME), 9);
	place_string(disp_buf, "sysop:", 59);
	place_string(disp_buf, user_check_flag(uSYSOP) ? "ON":"OFF", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "QTh:", 4);
	place_string(disp_buf, wp_get_field(call, wQTH), 9);
	place_string(disp_buf, "bbs:", 61);
	place_string(disp_buf, user_check_flag(uBBS) ? "YES":"NO", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "Zip:", 4);
	place_string(disp_buf, wp_get_field(call, wZIP), 9);
	place_string(disp_buf, "immune:", 58);
	place_string(disp_buf, user_check_flag(uIMMUNE) ? "YES":"NO", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "HOmebbs:", 0);
	place_string(disp_buf, wp_get_field(call, wHOME), 9);
	place_string(disp_buf, "logging:", 57);
	place_string(disp_buf, user_check_flag(uLOG) ? "ON":"OFF", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "PHone:", 2);
	place_string(disp_buf, user_get_field(uPHONE), 9);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "EMail:", 2);
	if(user_check_flag(uEMAIL))
		place_string(disp_buf, user_check_flag(uEMAILALL) ? "[ALL]":"[LOCAL]", 9);
	else
		place_string(disp_buf, "[OFF]", 9);
	place_string(disp_buf, user_get_field(uADDRESS), 17);
	PRINTF("%s\n", disp_buf);

	PRINTF("\n");


	PRINTF("\n");

	space_fill(disp_buf, 79);
	place_string(disp_buf, "Help:", 3);
	place_number(disp_buf, user_get_value(uHELP), 9);
	place_string(disp_buf, "DUPlex:", 29);
	place_string(disp_buf, user_check_flag(uHDUPLEX) ? "HALF":"FULL", 37);
	place_string(disp_buf, "SIGnature:", 55);
	place_string(disp_buf, user_check_flag(uSIGNATURE) ? "ON":"OFF", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "List:", 3);
	place_string(disp_buf, user_check_flag(uASCEND) ? "ASCENDING":"DESCENDING", 9);
	place_string(disp_buf, "NEwline:", 28);
	place_string(disp_buf, user_check_flag(uNEWLINE) ? "ON":"OFF", 37);
	place_string(disp_buf, "VAcation:", 56);
	place_string(disp_buf, user_check_flag(uVACATION) ? "ON":"OFF", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "REGEXP:", 1);
	place_string(disp_buf, user_check_flag(uREGEXP) ? "ON":"OFF", 9);
	place_string(disp_buf, "LINes:", 30);
	place_number(disp_buf, user_get_value(uLINES), 37);
	place_string(disp_buf, "UPpercase:", 55);
	place_string(disp_buf, user_check_flag(uUPPERCASE) ? "ON":"OFF", 66);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "BASE:", 3);
	place_number(disp_buf, user_get_value(uBASE), 9);
	PRINTF("%s\n", disp_buf);

	PRINTF("\n");

	if(!batch_mode) {
		PRINT("Next Page (q to quit)? ");
		if(get_yes_no_quit(YES) == QUIT) {
			user_focus(usercall);
			return;
		}
	}
	PRINTF("\n");

	PRINT("EXClude:\n");
	user_get_list(uEXCLUDE);
	usr_disp_list(Exclude);
	PRINTF("\n");

	PRINT("INClude:\n");
	user_get_list(uINCLUDE);
	usr_disp_list(Include);
	PRINTF("\n");

	if(!batch_mode) {
		PRINT("Next Page (q to quit)? ");
		if(get_yes_no_quit(YES) == QUIT) {
			user_focus(usercall);
			return;
		}
	}
	PRINT("\n");

	space_fill(disp_buf, 79);
	place_string(disp_buf, "TNC:", 5);
	place_string(disp_buf, user_get_field(uTNC), 10);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "COMputer:", 0);
	place_string(disp_buf, user_get_field(uCOMPUTER), 10);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "RIg:", 5);
	place_string(disp_buf, user_get_field(uRIG), 10);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "SOFtware:", 0);
	place_string(disp_buf, user_get_field(uSOFTWARE), 10);
	PRINTF("%s\n", disp_buf);

	space_fill(disp_buf, 79);
	place_string(disp_buf, "FReq:", 4);
	place_string(disp_buf, user_get_field(uFREQ), 10);
	PRINTF("%s\n", disp_buf);

	PRINT("\n");

	for(i=0; i<10; i++)
		PRINTF(" MAcro %d: %s\n", i, user_get_macro(i));

	PRINT("\n");

	if(!batch_mode) {
		PRINT("Next Page (q to quit)? ");
		if(get_yes_no_quit(YES) == QUIT) {
			user_focus(usercall);
			return;
		}
	}
	PRINT("\n");

	PRINTF("       Type      Name   Allow  Cnt      First seen           Last seen\n");

	userd_fetch_multi("PORT", usr_disp_ports);

	if(gated_open() == OK) {
		PRINTF("\nEmail accepted from:\n");
		sprintf(field, "USER %s", call);
		if(gated_fetch_multi(field, usr_disp_gateallow) == ERROR)
			PRINTF("\tNONE\n");
		gated_close();
	}
	user_focus(usercall);
}

