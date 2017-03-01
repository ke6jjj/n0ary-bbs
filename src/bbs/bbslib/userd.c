#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"


struct UserdCommands UserdCmds[] = {
	{ uADDRESS,		"ADDRESS",	uString, LenEMAIL,	0 },
	{ uASCEND,		"ASCEND",	uToggle, 0,			FALSE },
	{ uAPPROVED,	"APPROVED",	uToggle, 0,			TRUE },
	{ uBASE,		"BASE",		uNumber, 0,			0 },
	{ uBBS,			"BBS",		uToggle, 0,			FALSE },
	{ uCOMPUTER,	"COMPUTER",	uString, LenEQUIP,	0 },
	{ uCOUNT,		"COUNT",	uNumber, 0,			0 },
	{ uEMAIL,		"EMAIL",	uToggle, 0,			FALSE },
	{ uEMAILALL,	"EMAILALL",	uToggle, 0,			FALSE },
	{ uEXCLUDE,		"EXCLUDE",	uList,   LenINCLUDE,	0 },
	{ uFREQ,		"FREQ",		uString, LenFREQ,	0 },
	{ uHDUPLEX,		"HDUPLEX",	uToggle, 0,			FALSE },
	{ uHELP,		"HELP",		uNumber, 0,			3 },
	{ uIMMUNE,		"IMMUNE",	uToggle, 0,			FALSE },
	{ uINCLUDE,		"INCLUDE",	uList,   LenINCLUDE,	0 },
	{ uLINES,		"LINES",	uNumber, 0,			20 },
	{ uLNAME,		"LNAME",	uString, LenLNAME,	0 },
	{ uLOG,			"LOG",		uToggle, 0,			FALSE },
	{ uMACRO,		"MACRO",	uString, LenMACRO,	0 },
	{ uMESSAGE,		"MESSAGE",	uNumber, 0,			0 },
	{ uNEWLINE,		"NEWLINE",	uToggle, 0,			FALSE },
	{ uNONHAM,		"NONHAM",	uToggle, 0,			FALSE },
	{ uNUMBER,		"NUMBER",	uNumber, 0,			0 },
	{ uPHONE,		"PHONE",	uString, LenPHONE,	0 },
	{ uPORT,		"PORT",		uToggle, 0,			TRUE },
	{ uREGEXP,		"REGEXP",	uToggle, 0,			FALSE },
	{ uRIG,			"RIG",		uString, LenEQUIP,	0 },
	{ uSIGNATURE,	"SIGNATURE",uToggle, 0,			FALSE },
	{ uSOFTWARE,	"SOFTWARE",	uString, LenEQUIP,	0 },
	{ uSTATUS,		"STAT",		uCmd, 0,			FALSE },
	{ uSYSOP,		"SYSOP",	uToggle, 0,			FALSE },
	{ uTNC,			"TNC",		uString, LenEQUIP,	0 },
	{ uUPPERCASE,	"UPPERCASE",uToggle, 0,			FALSE },
	{ uVACATION,	"VACATION",	uToggle, 0,			FALSE },
	{ uAGE,			"AGE",		uCmd, 0,			FALSE },
	{ uCALL,		"CALL",		uCmd, 0,			FALSE },
	{ uCLEAR,		"CLEAR",	uCmd, 0,			FALSE },
	{ uCREATE,		"CREATE",	uCmd, 0,			FALSE },
	{ uFLUSH,		"FLUSH",	uCmd, 0,			FALSE },
	{ uKILL,		"KILL",		uCmd, 0,			FALSE },
	{ uLOCATE,		"LOCATE",	uCmd, 0,			FALSE },
	{ uLOGIN,		"LOGIN",	uCmd, 0,			FALSE },
	{ uSEARCH,		"SEARCH",	uCmd, 0,			FALSE },
	{ uSET,			"SET",		uCmd, 0,			FALSE },
	{ uSHOW,		"SHOW",		uCmd, 0,			FALSE },
	{ uSUFFIX,		"SUFFIX",	uCmd, 0,			FALSE },
	{ uUSER,		"USER",		uCmd, 0,			FALSE },
	{ 0,			NULL,		0,    0,			0 }
};
static int
	userd_sock = ERROR;

int
userd_open(void)
{
	int port;

	port = atoi(bbsd_get_variable("USERD_PORT"));
	if((userd_sock = socket_open(Bbs_Host, port)) == ERROR)
		return ERROR;

		/* read version number of userd, ignore for now */

	if(userd_read() == NULL)
		return ERROR;

	return OK;
}

void
userd_close(void)
{
	close(userd_sock);
	userd_sock = ERROR;
}

char *
userd_read(void)
{
	static char buf[1024];

	switch(socket_read_line(userd_sock, buf, 1023, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[1023] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		error_log("userd_read: error reading from socket");
		return NULL;
	}
	return buf;
}

char *
userd_fetch(char *cmd)
{
	char *c, buf[1024];

	if(userd_sock == ERROR)
		return NULL;

	strncpy(buf, cmd, 1023);
	buf[1023] = 0;
	strcat(buf, "\n");
	socket_raw_write(userd_sock, buf);
	c = userd_read();
	return c;
}

int
userd_fetch_multi(char *cmd, void (*callback)(char *s))
{
	char *c, buf[1024];

	if(userd_sock == ERROR)
		return 0;

	strncpy(buf, cmd, 1023);
	buf[1023] = 0;
	strcat(buf, "\n");
	socket_raw_write(userd_sock, buf);

	while(TRUE) {
		if((c = userd_read()) == NULL) {
			userd_close();
			return ERROR;
		}

		if(!strncmp(c, "NO,", 3))
			return ERROR;

		if(c[0] == '.' && c[1] == 0)
			break;

		callback(c);
	}
	return OK;
}
