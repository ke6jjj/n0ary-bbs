#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

static int
	logd_sock = ERROR;


static char *
logd_read(void)
{
	static char buf[80];

	switch(socket_read_line(logd_sock, buf, 80, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[79] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		error_log("bbsd_read: error reading from socket");
		return NULL;
	}
	return buf;
}

int
logd_open(char *whoami)
{
	int port;

	if(logd_sock != ERROR)
		return ERROR;

	port = atoi(bbsd_get_variable("LOGD_PORT"));
	if((logd_sock = socket_open(Bbs_Host, port)) == ERROR)
		return ERROR;

		/* read version number of logd, ignore for now */

	if(logd_read() == NULL)
		return ERROR;

	logd(whoami);
	return OK;
}

void
logd(char *string)
{
	char buf[4096];
	if(logd_sock == ERROR)
		return;

	sprintf(buf, "%s\n", string);
	socket_raw_write(logd_sock, buf);
}

void
logd_close(void)
{
	close(logd_sock);
	logd_sock = ERROR;
}

void
logd_stamp(char *whoami, char *string)
{
	if(logd_sock == ERROR) {
		logd_open(whoami);
		logd(string);
		logd_close();
	} else
		logd(string);
}

