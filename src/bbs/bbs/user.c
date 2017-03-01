#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef SUNOS
#include <regex.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tokens.h"
#include "message.h"
#include "user.h"
#include "wp.h"
#include "vars.h"
#include "function.h"
#include "version.h"

char
	include[LenINCLUDE],
	exclude[LenINCLUDE],
	usercall[LenCALL],
	last_login[80];

long
	UserLoaded = FALSE,
	ImSysop = FALSE,
	ImBBS = FALSE,
	ImNew = FALSE,
	ImLogging = FALSE,
	IGavePassword = FALSE,
	ISupportHloc = FALSE,
	ISupportBID = FALSE,
	PortNumber = ERROR;

long
	MyHelpLevel = 3,
	Lines = 0,
	Base = 0,
	ImSuspect = FALSE, 
	NeedsNewline = FALSE,
	Cnvrt2Uppercase = FALSE,
	ImAllowedSysop = FALSE,
	ImAscending = FALSE, 
	ImDescending = FALSE,
	ImHalfDuplex = FALSE,
	ImRegExp = FALSE,
	ImPotentialBBS = FALSE,
	ImNonHam = FALSE;

time_t
	last_login_time = 0;

struct IncludeList
	*Include = NULL,
	*Exclude = NULL;

extern long
	inactivity_timer; 

extern int
	debug_level;

void
user_refresh(void)
{
	user_focus(usercall);
	ImSuspect = !user_check_flag(uAPPROVED);
	NeedsNewline = user_check_flag(uNEWLINE);
	Cnvrt2Uppercase = user_check_flag(uUPPERCASE);
	ImAllowedSysop = user_check_flag(uSYSOP);
	ImHalfDuplex = user_check_flag(uHDUPLEX);
	ImPotentialBBS = user_check_flag(uBBS);
	ImNonHam = user_check_flag(uNONHAM);
	ImRegExp = user_check_flag(uREGEXP);
	ImAscending = user_check_flag(uASCEND);
	ImDescending = !ImAscending;
	Lines = user_get_value(uLINES);
	Base = user_get_value(uBASE);
	MyHelpLevel = (ImBBS) ? 0 : user_get_value(uHELP);
}

void
user_refresh_msg(void)
{
	ImRegExp = user_check_flag(uREGEXP);
	ImAscending = user_check_flag(uASCEND);
	ImDescending = !ImAscending;
	user_get_list(uINCLUDE);
	user_get_list(uEXCLUDE);
}

int
user_allowed_on_port(char *via)
{
	char cmd[80];
	char *result;

	sprintf(cmd, "PORT %s", via);
	result = userd_fetch(cmd);
	if(!strncmp(result, "ON", 2))
		return TRUE;
	return FALSE;
}
	
int
user_check_flag(int token)
{
	struct UserdCommands *uc = UserdCmds;
	char *result;

	if(!UserLoaded)
		return FALSE;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_check_flag(%d) ... internal error [token not found]\n", token);
			return FALSE;
		}
	}

	if(uc->type != uToggle) {
		PRINTF("user_check_flag(%d) ... internal error [not a toggle]\n", token);
		return FALSE;
	}

	if(!UserLoaded)
		return (int)uc->def;

	result = userd_fetch(uc->key);
	if(!strncmp(result, "OFF", 3))
		return FALSE;
	if(!strncmp(result, "ON", 2))
		return TRUE;

	PRINTF("user_check_flag(%s) ... internal error [%s]\n", uc->key, result);
	return FALSE;
}

int
user_focus(char *call)
{
	char *result;
	
	userd_fetch("FLUSH");
	result = userd_fetch(call);
	if(!strncmp(result, "NO,", 3)) {
		UserLoaded = FALSE;
		return ERROR;
	}

	UserLoaded = TRUE;
	return OK;
}

int
user_cmd(int token)
{
	struct UserdCommands *uc = UserdCmds;
	char *result;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_cmd(%d) ... internal error [token not found]\n",
				token);
			return ERROR;
		}
	}

	if(uc->type != uCmd) {
		PRINTF("user_cmd(%d) ... internal error [not a toggle]\n", token);
		return ERROR;
	}

	result = userd_fetch(uc->key);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_cmd(%s) ... internal error [%s]\n", uc->key, result);
		return ERROR;
	}
	return OK;
}

void
user_login(char *via)
{
	char cmd[80];
	sprintf(cmd, "LOGIN %s", via);
	userd_fetch(cmd);
}

void
user_flag(int token, char *toggle)
{
	struct UserdCommands *uc = UserdCmds;
	char cmd[80], *result;

	if(!UserLoaded)
		return;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_flag(%d, %s) ... internal error [token not found]\n",
				token, toggle);
			return;
		}
	}

	if(uc->type != uToggle) {
		PRINTF("user_flag(%d, %s) ... internal error [not a toggle]\n", token, toggle);
		return;
	}

	sprintf(cmd, "%s %s", uc->key, toggle);
	result = userd_fetch(cmd);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_flag(%s) ... internal error [%s]\n", cmd, result);
		return;
	}
}

void
user_clr_flag(int token)
{
	user_flag(token, "OFF");
}
	
void
user_set_flag(int token)
{
	user_flag(token, "ON");
	user_refresh();
}

void
user_set_macro(int number, char *s)
{
	char buf[LenMACRO], *result, cmd[256];

	if(!UserLoaded)
		return;

	strncpy(buf, s, LenMACRO);
	buf[LenMACRO-1] = 0;

	if(*s == 0)
		sprintf(cmd, "CLEAR MACRO %d", number);
	else
		sprintf(cmd, "MACRO %d %s", number, buf);

	result = userd_fetch(cmd);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_set_macro(%s) ... internal error [%s]\n", cmd, result);
		return;
	}
}

char *
user_get_macro(int number)
{
	char cmd[80];
	if(!UserLoaded)
		return NULL;

	sprintf(cmd, "MACRO %d", number);
	return userd_fetch(cmd);
}

struct IncludeList **List = NULL;

static void
user_make_list(char *s)
{
	char key, str[80];
	struct IncludeList *t;
	int first = TRUE;

	while(*s) {
		switch(*s) {
		case '>':
		case '<':
		case '@':
		case '&':
			key = *s++;
			break;
		default:
			key = '>';
		}

		if(*s == 0)
			return;

		strncpy(str, get_string(&s), 80);

		t = malloc_struct(IncludeList);
		if(first) {
			t->next = *List;
			first = FALSE;
		} else {
			t->comp = *List;
			t->next = (*List)->next;
		}
		*List = t;

		t->key = key;
		str[9] = 0;
		strcpy(t->str, str);
	}
}

static void
free_user_list(struct IncludeList **list)
{
	struct IncludeList *t;

	while(*list) {
		t = *list;
		NEXT((*list));
		free(t);
	}
}

int
user_get_list(int token)
{
	struct UserdCommands *uc = UserdCmds;
	int result;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_get_list(%d) ... internal error [token not found]\n", token);
			return ERROR;
		}
	}

	if(uc->type != uList) {
		PRINTF("user_get_list(%d) ... internal error [not a list]\n", token);
		return ERROR;
	}

	if(!UserLoaded)
		return OK;

	switch(token) {
	case uEXCLUDE:
		List = &Exclude;
		break;
	case uINCLUDE:
		List = &Include;
		break;
	}
	free_user_list(List);

	result = userd_fetch_multi(uc->key, user_make_list);
	return result;
}

char *
user_get_field(int token)
{
	struct UserdCommands *uc = UserdCmds;
	char *result;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_get_field(%d) ... internal error [token not found]\n", token);
			return "";
		}
	}

	if(uc->type != uString) {
		PRINTF("user_get_field(%d) ... internal error [not a string]\n", token);
		return "";
	}

	if(!UserLoaded)
		return (char *)uc->def;

	result = userd_fetch(uc->key);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_get_field(%s) ... internal error [%s]\n", uc->key, result);
		return "";
	}

	return result;
}

void
user_set_field(int token, char *s)
{
	struct UserdCommands *uc = UserdCmds;
	char buf[LenMAX], cmd[256], *result;

	if(!UserLoaded)
		return;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_set_field(%d) ... internal error [token not found]\n", token);
			return;
		}
	}

	if(uc->type != uString && uc->type != uList) {
		PRINTF("user_set_field(%d) ... internal error [not a string or list]\n", token);
		return;
	}

	if(*s) {
		strncpy(buf, s, uc->length);
		buf[uc->length-1] = 0;
		sprintf(cmd, "%s %s", uc->key, buf);
	} else
		sprintf(cmd, "CLEAR %s", uc->key);

	result = userd_fetch(cmd);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_set_field(%s) ... internal error [%s]\n", cmd, result);
		return;
	}
}

void
user_set_value(int token, long val)
{
	struct UserdCommands *uc = UserdCmds;
	char cmd[80], *result;

	if(!UserLoaded)
		return;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_set_value(%d) ... internal error [token not found]\n", token);
			return;
		}
	}

	if(uc->type != uNumber) {
		PRINTF("user_set_value(%d) ... internal error [not a number]\n", token);
		return;
	}

	snprintf(cmd, sizeof(cmd), "%s %ld", uc->key, val);
	result = userd_fetch(cmd);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_set_value(%s) ... internal error [%s]\n", cmd, result);
		return;
	}
}

long
user_get_value(int token)
{
	struct UserdCommands *uc = UserdCmds;
	char *result;

	while(uc->token != token) {
		uc++;
		if(uc->token == 0) {
			PRINTF("user_get_value(%d) ... internal error [token not found]\n", token);
			return ERROR;
		}
	}

	if(uc->type != uNumber) {
		PRINTF("user_get_value(%d) ... internal error [not a number]\n", token);
		return ERROR;
	}

	if(!UserLoaded)
		return uc->def;

	result = userd_fetch(uc->key);

	if(!strncmp(result, "NO,", 3)) {
		PRINTF("user_set_value(%s) ... internal error [%s]\n", uc->key, result);
		return ERROR;
	}

	return atoi(result);
}

void
user_create(void)
{
	char cmd[80], *p, buf[80];

	sprintf(cmd, "CREATE %s", usercall);
	strcpy(buf, userd_fetch(cmd));
	if(!strncmp(buf, "NO,", 3)) {
		PRINTF("Unable to create user\n");
		exit(1);
	}

	UserLoaded = TRUE;

	if(parse_callsign(usercall) == CALLisSUSPECT) {
		user_clr_flag(uAPPROVED);
		user_set_flag(uNONHAM);
	}

	sprintf(cmd, "USER %s", usercall);
	strcpy(buf, userd_fetch(cmd));

	if(!strncmp(buf, "NO,", 3)) {
		UserLoaded = FALSE;
		last_login[0] = 0;
		return;
	}

	p = buf;
	NextChar(p);	/* start of call */
	NextSpace(p);
	NextChar(p);	/* start of count */
	NextSpace(p);
	NextChar(p);	/* start of port */

	strcpy(cmd, get_string(&p));
	last_login_time = get_number(&p);

	sprintf(last_login, "Last login via %s on %s",
		cmd, ctime(&last_login_time));

	strcpy(buf, userd_fetch(usercall));
	if(!strncmp(buf, "NO,", 3)) {
		UserLoaded = FALSE;
		last_login[0] = 0;
		last_login_time = 0;
		return;
	}

	user_refresh();
}

int
user_open(void)
{
	char cmd[80], *p, buf[80];

	if(userd_open() == ERROR) {
		PRINTF("Unable to connect to user daemon\n");
		exit(1);
	}

	sprintf(cmd, "USER %s", usercall);
	strcpy(buf, userd_fetch(cmd));

	if(!strncmp(buf, "NO,", 3)) {
		UserLoaded = FALSE;
		last_login[0] = 0;
		return ERROR;
	}

	p = buf;
	NextChar(p);	/* start of call */
	NextSpace(p);
	NextChar(p);	/* start of count */
	NextSpace(p);
	NextChar(p);	/* start of port */

	strcpy(cmd, get_string(&p));
	last_login_time = get_number(&p);

	sprintf(last_login, "Last login via %s on %s",
		cmd, ctime(&last_login_time));

	strcpy(buf, userd_fetch(usercall));
	if(!strncmp(buf, "NO,", 3)) {
		UserLoaded = FALSE;
		last_login[0] = 0;
		last_login_time = 0;
		return ERROR;
	}

	UserLoaded = TRUE;
	return OK;
}

char *
user_by_number(int number)
{
	char cmd[80];
	sprintf(cmd, "LOCATE %d", number);
	return userd_fetch(cmd);
}

static int
check_homebbs(void)
{
	char *p, str[80];

	strcpy(str, wp_get_field(usercall, wHOME));

	if(str[0] != '?') {
		if(wp_test_field(str, wHLOC) == ERROR) {
PRINTF("Your choice for a homebbs [%s] is not in my local white pages\n", str);
PRINT("as being a valid BBS. Please type INFO HOMEBBS and verify that\n");
PRINT("have indeed made a proper choice.\n");
		}
		return FALSE;
	}

PRINT("All bbs users have to claim a home bbs. This is necessary for you\n");
PRINT("to reliably receive mail. You have to choose a full service bbs not\n");
PRINT("your personal tnc. If you are located in the San Francisco area you are\n");
PRINTF("welcome to take %s as your home. If you are confused go ahead\n", Bbs_Call);
PRINTF("and accept %s for now and do the following at the prompt:\n", Bbs_Call);
PRINT("  INFO HOMEBBS\n\n");

	while(TRUE) {
		PRINTF("Please choose a homebbs [%s]:\n", Bbs_Call);
		GETnSTRdef(str, LenHOME, AllUpperCase, Bbs_Call);

		if((p = (char*)index(str, '-')) != NULL) {
			PRINTF(
				"The %s is not required on the homebbs, we will ignore it\n", p);
			*p = 0;
		}

		if(!isCall(str)) {
			PRINTF( "%s doesn't seem to be a valid callsign, try again\n", str);
			continue;
		}

		if(wp_test_field(str, wHLOC) == ERROR) {
			PRINTF("%s isn't listed in my white pages as being a bbs,\n", str);
			PRINTF("this could be a problem.\n");
			return FALSE;
		}
		break;
	}

	wp_set_field(usercall, wHOME, wUSER, str);
	return TRUE;
}

static int
check_name(void)
{
	char *p, name[80];
	int bad, cnt, altered = FALSE;

	while(TRUE) {
		strcpy(name, wp_get_field(usercall, wFNAME));
		p = name;
		cnt = bad = 0;
		while(*p) {
			if(isdigit(*p++))
				bad = TRUE;
			cnt++;
		}
		if(bad)
			PRINTF("I suspect that your first name [%s] is wrong\n", name);

		if(bad || cnt == 0) {
			PRINT("Please enter your first name: \n");
			GETnSTR(name, LenFNAME, CapAsIs);
			wp_set_field(usercall, wFNAME, wUSER, name);
			altered = TRUE;
			continue;
		}

		strcpy(name, user_get_field(uLNAME));
		p = name;
		cnt = bad = 0;
		while(*p) {
			if(isdigit(*p++))
				bad = TRUE;
			cnt++;
		}
		if(bad)
			PRINTF("I suspect that your last name [%s] is wrong\n", name);

		if(bad || cnt == 0) {
			PRINT("Please enter your last name: \n");
			GETnSTR(name, LenLNAME, CapAsIs);
			user_set_field(uLNAME, name);
			altered = TRUE;
			continue;
		}

		break;
	}
	return altered;
}

static int
check_equip(void)
{
	char comp[LenEQUIP], rig[LenEQUIP], soft[LenEQUIP], tnc[LenEQUIP];

	strncpy(comp, user_get_field(uCOMPUTER), LenEQUIP-1);
	strncpy(rig, user_get_field(uRIG), LenEQUIP-1);
	strncpy(soft, user_get_field(uSOFTWARE), LenEQUIP-1);
	strncpy(tnc, user_get_field(uTNC), LenEQUIP-1);

	if(comp[0] && rig[0] && soft[0] && tnc[0])
		return FALSE;

PRINT("At times it is nice to know what kind of packet equipment you are\n");
PRINT("using. This helps resolve problems you may be having as well as\n");
PRINT("allowing me to locate people using the same type of equipment when\n");
PRINT("someone else has problems.\n\n");

	if(rig[0] == 0) {
		PRINT("What type of radio do you use for packet [None]:\n");
		GETnSTRdef(rig, LenEQUIP-1, CapAsIs, "None");
		rig[LenEQUIP-1] = 0;
		user_set_field(uRIG, rig);
	}
	if(soft[0] == 0) {
		PRINT("What software do you use for packet [None]:\n");
		GETnSTRdef(soft, LenEQUIP-1, CapAsIs, "None");
		soft[LenEQUIP-1] = 0;
		user_set_field(uSOFTWARE, soft);
	}
	if(tnc[0] == 0) {
		PRINT("What is the make and model of your tnc [None]:\n");
		GETnSTRdef(tnc, LenEQUIP-1, CapAsIs, "None");
		tnc[LenEQUIP-1] = 0;
		user_set_field(uTNC, tnc);
	}
	if(comp[0] == 0) {
		PRINT("What type of computer do you use for packet [None]:\n");
		GETnSTRdef(comp, LenEQUIP-1, CapAsIs, "None");
		comp[LenEQUIP-1] = 0;
		user_set_field(uCOMPUTER, comp);
	}
	return TRUE;
}

static int
check_phone(void)
{
	char phone[LenPHONE];

	strncpy(phone, user_get_field(uPHONE), LenPHONE-1);
	if(phone[0])
		return FALSE;

PRINT("At times I need to get ahold of a user for some reason. If your phone\n");
PRINT("number is not silent would you please input it into the system. If it\n");
PRINT("is unlisted just type UNLISTED.\n\n");

	PRINT("Please enter your phone number [Unlisted]:\n");
	GETnSTRdef(phone, LenPHONE, CapAsIs, "Unlisted");
	user_set_field(uPHONE, phone);
	return TRUE;
}

static int
check_freq(void)
{
	char freq[LenFREQ];

	strncpy(freq, user_get_field(uFREQ), LenFREQ-1);
	if(freq[0])
		return FALSE;

PRINT("To help other contact you for info we maintain a entry in the database\n");
PRINT("that tells where you can most commonly be found on the radio. If you\n");
PRINT("don't have a common place you can be found please indicate this also.\n");
PRINT("For example, here is my entry:\n");
PRINT("  443.775+/224.28-/145.62s all 100hz PL (LERA's Linked System)\n\n");

	PRINT("Please enter frequencies you normally monitor [None]:\n");
	GETnSTRdef(freq, LenFREQ, CapAsIs, "None");
	user_set_field(uFREQ, freq);
	return TRUE;
}

void
fill_in_blanks(void)
{
	int count = user_get_value(uCOUNT);

	if(!ImNonHam) {
		check_name();
		if(check_homebbs())
			return;
	}

	if(count > 10) {
		if(check_phone())
			return;
		if(check_freq())
			return;

		if(count > 20)
			if(check_equip())
				return;
	}
}

int
parse_bbssid(char *str, char *chksid)
{
	char *p, buf[1024];

	/* if chksid is not NULL then we should do a regular expression
	 * check to see if this sid matches the one in our Systems file.
	 * this is a level of protection added because the bbs appeared
	 * to be connecting to bbs' that didn't support HLOCs and BIDs.
	 */

	if(chksid != NULL) {
		if((re_comp(chksid)) != NULL) {
			struct text_line *tl = NULL;
			textline_append(&tl, 
				  "The SID presented in your Systems file doesn't appear");
			textline_append(&tl, 
                  "to be a valid regular expression.");
			textline_append(&tl, chksid);
			problem_report_textline(Bbs_Call, BBS_VERSION, usercall, tl);
			textline_free(tl);
			return ERROR;
		}
		if(re_exec(str) != 1) {
			struct text_line *tl = NULL;
			textline_append(&tl, 
				  "The bbs we have connected to returned a bid that was");
			textline_append(&tl, 
                  "different than the one supplied in the Systems file.");
			sprintf(buf, "\texp: %s", chksid);
			textline_append(&tl, buf);
			sprintf(buf, "\tgot: %s", str);
			textline_append(&tl, buf);
			problem_report_textline(Bbs_Call, BBS_VERSION, usercall, tl);
			textline_free(tl);
			return ERROR;
		}
	}

	
	strcpy(buf, str);

	ImBBS = FALSE;
	ISupportHloc = FALSE;
	ISupportBID = FALSE;

	if((p = (char *)index(buf, ']')) == NULL)
		return OK;
	*p = 0;

	if((p = (char *)rindex(buf, '-')) == NULL)
		return OK;

	while(*p) {
		switch(*p++) {
		case 'H':
			ISupportHloc = TRUE;
			break;
		case '$':
			ISupportBID = TRUE;
			break;
		}
	}

	ImBBS = TRUE;
	MyHelpLevel = 0;
	logd_open(usercall);
	inactivity_timer = FALSE;

	if(ImPotentialBBS) {
		char prefix[80];
		sprintf(prefix, "<%s: ", usercall);
		bbsd_prefix_msg(prefix);
		bbsd_msg(" ");

		msg_BbsMode();
		build_full_message_list();
	}
	return OK;
}
