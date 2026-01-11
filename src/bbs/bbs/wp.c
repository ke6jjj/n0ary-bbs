#include <stdio.h>
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
#include "wp.h"
#include "server.h"
#include "vars.h"

static char
	*calllist[100];

static int
	callcnt;


int
wp_open(void)
{
	if(wpd_open() == ERROR) {
		PRINTF("Error connecting to wpd\n");
		return ERROR;
	}
	return OK;
}

char *
wp_fetch(char *cmd)
{
	char *c;

	if(wp_open() == ERROR)
		return NULL;
	c = wpd_fetch(cmd);
	wpd_close();

	return c;
}

static void
wp_callback(char *s)
{
	PRINTF("%s\n", s);
}

int
wp_fetch_multi(char *cmd)
{
	if(wp_open() == ERROR) 
		return ERROR;
	wpd_fetch_multi(cmd, wp_callback);
	wpd_close();
	return OK;
}

int
wp_test_field(char *call, int token)
{
	struct WpdCommands *wc = WpdCmds;
	char *result, cmd[256];

	while(wc->token != token) {
		wc++;
		if(wc->token == 0) {
			PRINTF("wp_test_field(%d) ... internal error [token not found]\n",
				token);
			return FALSE;
		}
	}

	sprintf(cmd, "%s %s", call, wc->key);
	result = wp_fetch(cmd);

	if(!strncmp(result, "NO,", 3) || *result == '?')
		return ERROR;

	return OK;
}

int
wp_cmd(char *cmd)
{
	char *c = wp_fetch(cmd);

	if(c == 0 || (c[0] == 'N' && c[1] == 'O'))
		return ERROR;
	return OK;
}

int 
wp_create_bbs(char *call)
{
	char cmd[80];

	if(wp_open() == ERROR)
		return ERROR;

	sprintf(cmd, "BBS CREATE %s", call);
	wp_cmd(cmd);

	wpd_close();
	return OK;
}

int
wp_create_user(char *call)
{
	if(wp_open() == ERROR)
		return ERROR;
	if(wp_cmd(call)) {
		char cmd[80];
		sprintf(cmd, "CREATE %s", call);
		wp_cmd(cmd);
	}
	wpd_close();
	return OK;
}

/* ============================== */

static int
wp_usage(void)
{
	PRINT("WP Command\n");
	PRINT("  User level:\n");
	PRINT("    WP calls               lookup users home bbs\n");
	PRINT("    WP SEARCH st           show bbs's in supplied state\n");
	PRINT("    WP HOME LIST bbs       show home users at bbs\n");
	PRINT("\n");
	PRINT("  Sysop level:\n");
	PRINT("    WP ALTERED             calls needing to be updated\n");
	PRINT("    WP ADD calls           create a wp entry\n");
	PRINT("    WP GENERATE            send update to regional server\n");
	PRINT("    WP REFRESH             gen update for all users\n");
	PRINT("    WP EDIT call           edit a calls wp fields\n");
	PRINT("    WP TOUCH calls         force an update for a user\n");
	PRINT("    WP KILL calls          remove call from wp\n");
	PRINT("    WP BBS KILL calls      remove call from bbs wp\n");
	PRINT("    WP CHECK               do sanity checks across WP\n");
	PRINT("    WP FIX                 fix entries found invalid in CHECK\n");
	PRINT("    WP WRITE               write user wp in ascii form to disk\n");
	PRINT("    WP BBS WRITE           write bbs wp in ascii form to disk\n");
	PRINT("    WP READ                read wp ascii update from disk\n");
	PRINT("\n");

	return OK;
}

/*ARGSUSED*/
static int
wp_search(char *str)
{
#if 0
	char buf[80];

	case_string(str, AllUpperCase);
	sprintf(buf, "SEARCH HLOC %s", str);
	wp_fetch_multi(buf);
#endif
	return OK;
}

char *
wp_get_field(char *call, int field)
{
	struct WpdCommands *wc = WpdCmds;
	char *result, cmd[80];

	while(wc->token != field) {
		wc++;
		if(wc->token == 0) {
			PRINTF("wp_get_field(%d) ... internal error [field not found]\n", field);
			return "";
		}
	}

	if(wc->type != wString) {
		PRINTF("wp_get_field(%d) ... internal error [not a string]\n", field);
		return "";
	}

	sprintf(cmd, "%s %s", call, wc->key);
	result = wp_fetch(cmd);

	if(!strncmp(result, "NO,", 3))
		return "";

	return result;
}

int
wp_set_field(char *call, int field, int level, char *value)
{
	struct WpdCommands *wc = WpdCmds;
	char *level_p, cmd[256];
	char buf[LenMAX];

	while(wc->token != level) {
		wc++;
		if(wc->token == 0) {
			PRINTF("wp_set_field(%d) ... internal error [level not found]\n",
				field);
			return ERROR;
		}
	}
	level_p = wc->key;

	wc = WpdCmds;
	while(wc->token != field) {
		wc++;
		if(wc->token == 0) {
			PRINTF("wp_set_field(%d) ... internal error [field not found]\n",
				field);
			return ERROR;
		}
	}

	if(wc->type != wString) {
		PRINTF("wp_set_field(%d) ... internal error [not a string]\n", field);
		return ERROR;
	}

	strncpy(buf, value, wc->length);
	buf[wc->length-1] = 0;
	sprintf(cmd, "%s %s %s %s", call, level_p, wc->key, buf);
	wp_fetch(cmd);

	return OK;
}

/*ARGSUSED*/
int
wp_home_count(char *call)
{
	return 0;
}

int
wp_set_users_homebbs(char *call, char *homebbs, int level)
{
	/* before we set the home bbs we need to check to see if it is
	 * indeed a valid bbs.
	 */

	if(wp_cmd(homebbs) != OK && !ImBBS) {
		PRINTF("** The bbs you have chosen %s is not known to this bbs.\n", homebbs);
		PRINTF("** There are two reasons this can occur. First, the bbs you chose\n");
		PRINTF("** may be knew and not in this bbs's white pages. Second, the bbs is\n");
		PRINTF("** not a full service bbs. Please type INFO HOMEBBS to learn more\n");
		PRINTF("** about what a home bbs is.\n");
	}
	
	wp_set_field(call, wHOME, level, homebbs);
	return OK;
}

static int
scan_call_list(void)
{
	int i;

	for(i=0; i<callcnt; i++)
		PRINTF("%s\n", wp_fetch(calllist[i]));
	return OK;
}

int
wp(void)
{
	struct TOKEN *t = TokenList;
	callcnt = 0;

	NEXT(t);

	switch(t->token) {
	case END:
		return wp_usage();
	case SEARCH:
		return wp_search(t->next->lexem);
	}

	while(t->token != END) {
		switch(t->token) {
		case COMMA:
			break;

		case WORD:
		case NUMBER:
			case_string(t->lexem, AllUpperCase);
			calllist[callcnt++] = t->lexem;
			if(callcnt >= 100) {
				PRINT("More than 100 calls specified, WP command aborted\n");
				return OK;
			}
			break;
		default:
			return bad_cmd(-1, t->location);
		}

		NEXT(t);
	}

	if(callcnt != 0)
		scan_call_list();
	return OK;
}

void
wp_server(int num, char *mode)
{
	int result = ERROR;
	struct text_line *tl;
	struct msg_dir_entry *m;

	case_string(mode, AllUpperCase);

	InServer = 'W';
	strcpy(usercall, "WPSERV");
	ImSysop = TRUE;
	IGavePassword = TRUE;

	if(msgd_open())
		return;

	msg_LoginUser(usercall);
	msg_SysopMode();
	build_full_message_list();

	m = msg_locate(num);
	msg_ReadBody(m);
	if(strncmp(m->from.name.str, Bbs_Call, strlen(Bbs_Call))) {

		if(!strcmp(mode, "WP UPDATE")) {
			char cmd[256];

			tl = m->body;
			while(tl) {
				if(!strncmp(tl->s, "On ", 3)) {
					sprintf(cmd, "UPLOAD %s", tl->s);
					result = wp_cmd(cmd);
					if(result != OK)
						break;
				}
				NEXT(tl);
			}
		}
	}
}

void
wp_seen(char *call)
{
	char buf[80];
	if(!isCall(call))
		return;

	sprintf(buf, "%s SEEN 0", call);
	wp_cmd(buf);
}
