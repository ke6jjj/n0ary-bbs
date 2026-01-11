#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

struct WpdCommands WpdCmds[] = {
	{ wADDRESS,	"ADDRESS",	wCmd,	0 },
	{ wBBS,		"BBS",		wCmd,	0 },
	{ wCALL,	"CALL",		wCmd,	0 },
	{ wCHANGED,	"CHANGED",	wCmd,	0 },
	{ wCREATE,	"CREATE",	wCmd,	0 },
	{ wFNAME,	"FNAME",	wString,	LenFNAME },
	{ wGUESS,	"GUESS",	wLevel,	0 },
	{ wHLOC,	"HLOC",		wString,	LenHLOC },
	{ wHOME,	"HOME",		wString,	LenHOME },
	{ wKILL,	"KILL",		wCmd,	0 },
	{ wLEVEL,	"LEVEL",	wLevel,	0 },
	{ wQTH,		"QTH",		wString,	LenQTH },
	{ wSEARCH,	"SEARCH",	wCmd,	0 },
	{ wSEEN,	"SEEN",		wCmd,	0 },
	{ wSHOW,	"SHOW",		wCmd,	0 },
	{ wSTAT,	"STAT",		wCmd,	0 },
	{ wSYSOP,	"SYSOP",	wLevel,	0 },
	{ wUPDATE,	"UPDATE",	wCmd,	0 },
	{ wUPLOAD,	"UPLOAD",	wCmd,	0 },
	{ wUSER,	"USER",		wLevel,	0 },
	{ wWRITE,	"WRITE",	wCmd,	0 },
	{ wZIP,		"ZIP",		wString,	LenZIP },
	{ 0,		NULL,		0,		0 }
};

static int
	wpd_sock = ERROR;

int
wpd_open_specific(char *hostname, int port)
{
	char *host = hostname;

	if(wpd_sock != ERROR)
		return OK;

	if((wpd_sock = socket_open(host, port)) == ERROR)
		return ERROR;

		/* read version number of wpd, ignore for now */

	if(wpd_read() == NULL)
		return ERROR;

	return OK;
}

int
wpd_open(void)
{
	int port; 

	if(wpd_sock != ERROR)
		return OK;

	port = atoi(bbsd_get_variable("WPD_PORT"));
	if((wpd_sock = socket_open(Bbs_Host, port)) == ERROR)
		return ERROR;

		/* read version number of wpd, ignore for now */

	if(wpd_read() == NULL)
		return ERROR;

	return OK;
}

void
wpd_close(void)
{
	close(wpd_sock);
	wpd_sock = ERROR;
}

char *
wpd_read(void)
{
	static char buf[256];
	switch(socket_read_line(wpd_sock, buf, 255, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[255] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		log_error("wpd_read: error reading from socket");
		return NULL;
	}
	return buf;
}

char *
wpd_fetch(char *cmd)
{
	char *c, buf[256];

	if(wpd_sock == ERROR)
		return NULL;

	if(strlen(cmd) > 255)
		return "NO, line too long\n";

	sprintf(buf, "%s\n", cmd);
	socket_raw_write(wpd_sock, buf);
	c = wpd_read();
	return c;
}

int
wpd_fetch_multi(char *cmd, void (*callback)(char *s))
{
	char *c, buf[256];

	if(wpd_sock == ERROR)
		return 0;

	if(strlen(cmd) > 255)
		return ERROR;

	sprintf(buf, "%s\n", cmd);
	socket_raw_write(wpd_sock, buf);

	while(TRUE) {
		if((c = wpd_read()) == NULL) {
			wpd_close();
			return ERROR;
		}

		if(c[0] == '.' && c[1] == 0)
			break;

		callback(c);
	}
	return OK;
}
