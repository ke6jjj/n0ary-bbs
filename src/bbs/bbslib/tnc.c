#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

static int
	sock = ERROR;

static int
tnc_readline(char *buf, int cnt)
{
	switch(socket_read_line(sock, buf, cnt-1, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[cnt-1] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		error_log("tnc_readline: error reading from socket");
		return ERROR;
	}
	return OK;
}

int
tnc_set_ax25(struct ax25_params *ax25)
{
	char cmd[80];

	if(ax25->t1) {
		sprintf(cmd, "~S T1 %d\n", ax25->t1);
		socket_raw_write(sock, cmd);
	}
	if(ax25->t2) {
		sprintf(cmd, "~S T2 %d\n", ax25->t2);
		socket_raw_write(sock, cmd);
	}
	if(ax25->t3) {
		sprintf(cmd, "~S T3 %d\n", ax25->t3);
		socket_raw_write(sock, cmd);
	}
	if(ax25->maxframe) {
		sprintf(cmd, "~S MAXFRAME %d\n", ax25->maxframe);
		socket_raw_write(sock, cmd);
	}
	if(ax25->paclen) {
		sprintf(cmd, "~S PACLEN %d\n", ax25->paclen);
		socket_raw_write(sock, cmd);
	}
	if(ax25->pthresh) {
		sprintf(cmd, "~S PTHRESH %d\n", ax25->pthresh);
		socket_raw_write(sock, cmd);
	}
	if(ax25->n2) {
		sprintf(cmd, "~S N2 %d\n", ax25->n2);
		socket_raw_write(sock, cmd);
	}
	return OK;
}

int
tnc_connect(char *host, int port, char *dest, char *mycall)
{
	char cmd[80], result[1024];
	fd_set ready;
	struct timeval timeout;

	if((sock = socket_open(host, port)) == ERROR)
		return ERROR;

	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if(select(64, (void*)&ready, 0, 0, &timeout) == 0) {
		close(sock);
		return ERROR;
	}
		/* read in version and ignore */
	if(tnc_readline(cmd, 80) == ERROR) {
		close(sock);
		return ERROR;
	}

	sprintf(cmd, "~C%s>%s\n", mycall, dest);
	socket_raw_write(sock, cmd);

	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	timeout.tv_sec = 120;
	timeout.tv_usec = 0;

	if(select(64, (void*)&ready, 0, 0, &timeout) == 0) {
		close(sock);
		return ERROR;
	}

	if(tnc_readline(result, 1024) == ERROR)
		return ERROR;

	if(result[0] == '~' && result[1] == 'C')
		return sock;

	return ERROR;
}

