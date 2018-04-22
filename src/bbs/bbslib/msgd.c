#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

int msgd_debug_level = 0;

struct MsgdCommands MsgdCmds[] = {
	{ mACTIVE,	"ACTIVE" },
	{ mAGE,		"AGE" },
	{ mBBS,		"BBS" },
	{ mCATCHUP,	"CATCHUP" },
	{ mDEBUG,	"DEBUG" },
	{ mDISP,	"DISP" },
	{ mEDIT,	"EDIT" },
	{ mFLUSH,	"FLUSH" },
	{ mFORWARD,	"FORWARD" },
	{ mGROUP,	"GROUP" },
	{ mHOLD,	"HOLD" },
	{ mIMMUNE,	"IMMUNE" },
	{ mKILL,	"KILL" },
	{ mKILLH,	"KILLH" },
	{ mLIST,	"LIST" },
	{ mMINE,    "MINE" },
	{ mNORMAL,	"NORMAL" },
	{ mPARSE,	"PARSE" },
	{ mPENDING,	"PENDING" },
	{ mREAD,	"READ" },
	{ mREADH,	"READH" },
	{ mREADRFC,	"READRFC" },
	{ mREHASH,	"REHASH" },
	{ mROUTE,	"ROUTE" },
	{ mSEND,	"SEND" },
	{ mSET,		"SET" },
	{ mSINCE,   "SINCE" },
	{ mSTAT,   "STAT" },
	{ mSYSOP,	"SYSOP" },
	{ mUSER,	"USER" },
	{ mWHO,		"WHO" },
	{ mWHY,		"WHY" },
	{ 0,		NULL }
};

static int
	msgd_sock = ERROR;

void
msgd_debug(int level)
{
	msgd_debug_level = level;
}

int
msgd_open(void)
{
	int port = atoi(bbsd_get_variable("MSGD_PORT"));

	if((msgd_sock = socket_open(Bbs_Host, port)) == ERROR)
		return ERROR;

		/* read version number of msgd, ignore for now */

	if(msgd_read() == NULL)
		return ERROR;

	return OK;
}

void
msgd_close(void)
{
	close(msgd_sock);
	msgd_sock = ERROR;
}

char *
msgd_read(void)
{
	static char buf[256];
	switch(socket_read_line(msgd_sock, buf, sizeof(buf)-1, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[255] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		error_log("msgd_read: error reading from socket");
		return NULL;
	}

	if(msgd_debug_level)
		printf("R: %s\n", buf);
	return buf;
}

char *
msgd_cmd(char *cmd)
{
	char *c, buf[80];

	if(msgd_sock == ERROR)
		return NULL;

	sprintf(buf, "%s\n", cmd);

	if(msgd_debug_level)
		printf("W: %s", buf);

	socket_raw_write(msgd_sock, buf);
	c = msgd_read();
	return c;
}

char *
msgd_cmd_string(char *cmd, char *s)
{
	char *c, buf[80];

	if(msgd_sock == ERROR)
		return NULL;

	sprintf(buf, "%s %s\n", cmd, s);

	if(msgd_debug_level)
		printf("W: %s", buf);

	socket_raw_write(msgd_sock, buf);
	c = msgd_read();
	return c;
}

char *
msgd_cmd_num(char *cmd, int num)
{
	char *c, buf[80];

	if(msgd_sock == ERROR)
		return NULL;

	sprintf(buf, "%s %d\n", cmd, num);
	if(msgd_debug_level)
		printf("W: %s", buf);

	socket_raw_write(msgd_sock, buf);
	c = msgd_read();
	return c;
}

char *
msgd_cmd_textline(char *cmd, struct text_line *tl)
{
	char *c, buf[1024];

	sprintf(buf, "%s\n", cmd);
	if(msgd_debug_level)
		printf("W: %s", buf);
	socket_raw_write(msgd_sock, buf);

	while(tl) {
		if (!strcmp(tl->s, "."))
			sprintf(buf, "..\n");
		else
			sprintf(buf, "%.1022s\n", tl->s);
		if(msgd_debug_level)
			printf("W: %s", buf);

		socket_raw_write(msgd_sock, buf);
		NEXT(tl);
	}
	if(msgd_debug_level)
		printf("W: .\n");

	socket_raw_write(msgd_sock, ".\n");
	c = msgd_read();
	return c;
}

int
msgd_fetch_multi(char *cmd, void (*callback)(char *s))
{
	char *c, buf[80];
	int first_time = TRUE;

	if(msgd_sock == ERROR)
		return 0;

	sprintf(buf, "%s\n", cmd);
	if(msgd_debug_level)
		printf("W: %s", buf);
	socket_raw_write(msgd_sock, buf);

	while(TRUE) {
		if((c = msgd_read()) == NULL) {
			msgd_close();
			return ERROR;
		}

		if(first_time) {
			if(!strncmp(c, "NO,", 3))
				return ERROR;
			first_time = FALSE;
		}

		if(c[0] == '.') {
			if (c[1] == 0)
				break;
			else if (c[1] == '.')
				/* Quoted dot, eat it. */
				c++;
		}

		callback(c);
	}
	return OK;
}

int
msgd_fetch_textline(char *cmd, struct text_line **tl)
{
	char *c, buf[80];
	int first_time = TRUE;

	if(msgd_sock == ERROR)
		return 0;

	sprintf(buf, "%s\n", cmd);
	if(msgd_debug_level)
		printf("W: %s", buf);
	socket_raw_write(msgd_sock, buf);

	while(TRUE) {
		if((c = msgd_read()) == NULL) {
			msgd_close();
			return ERROR;
		}

		if(first_time) {
			if(!strncmp(c, "NO,", 3))
				return ERROR;
			first_time = FALSE;
		}

		if(c[0] == '.') {
			if (c[1] == 0)
				break;
			else if (c[1] == '.')
				/* Quoted dot, eat it. */
				c++;
		}

		textline_append(tl, c);
	}
	return OK;
}

char *
msgd_xlate(int token)
{
	struct MsgdCommands *mc = MsgdCmds;

	while(mc->token && mc->token != token)
		mc++;
	return mc->key;
}

