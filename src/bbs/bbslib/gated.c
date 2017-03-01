#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

struct GatedCommands GatedCmds[] = {
	{ gADD,		"ADD" },
	{ gADDRESS,	"ADDRESS" },
	{ gDELETE,	"DELETE" },
	{ gGUESS,	"GUESS" },
	{ gSEEN,	"SEEN" },
	{ gSTAT,	"STAT" },
	{ gTOUCH,	"TOUCH" },
	{ gUSER,	"USER" },
	{ gWRITE,	"WRITE" },
	{ 0,		NULL }
};

static int
	gated_sock = ERROR;

int
gated_open(void)
{
	int port = atoi(bbsd_get_variable("GATED_PORT"));

	if((gated_sock = socket_open(Bbs_Host, port)) == ERROR)
		return ERROR;

		/* read version number of gated, ignore for now */

	if(gated_read() == NULL)
		return ERROR;

	return OK;
}

void
gated_close(void)
{
	close(gated_sock);
	gated_sock = ERROR;
}

char *
gated_read(void)
{
	static char buf[256];

	switch(socket_read_line(gated_sock, buf, 255, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[255] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		error_log("gated_read: error reading from socket");
		return NULL;
	}
	return buf;
}

char *
gated_fetch(char *cmd)
{
	char *c, buf[80];

	if(gated_sock == ERROR)
		return NULL;

	sprintf(buf, "%s\n", cmd);
	socket_raw_write(gated_sock, buf);
	c = gated_read();
	return c;
}

int
gated_fetch_multi(char *cmd, void (*callback)(char *s))
{
	char *c, buf[80];

	if(gated_sock == ERROR)
		return 0;

	sprintf(buf, "%s\n", cmd);
	socket_raw_write(gated_sock, buf);

	while(TRUE) {
		if((c = gated_read()) == NULL) {
			gated_close();
			return ERROR;
		}

		if(c[0] == '.' && c[1] == 0)
			break;

		if(!strncmp(c, "NO,", 3))
			return ERROR;
		callback(c);
	}
	return OK;
}
