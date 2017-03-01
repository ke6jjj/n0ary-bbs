#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "socket.h"
#include "version.h"

extern char versionc[80];
extern char *Bbs_Call;

int
accept_socket(int socket)
{
	fd_set ready;
	struct timeval to;
	int fd = -1;
	int result;

	FD_ZERO(&ready);
	FD_SET(socket, &ready);
	to.tv_sec = 0;
	to.tv_usec = 30;

	result = select(socket+1, (void*)&ready, (void*)0, (void*)0, &to);

	if(result < 0) {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "accept_socket", versionc,
				   "select call returned ERROR, exiting");
		exit(1);
	}

	if(result == 0)
		return ERROR;

	if(FD_ISSET(socket, &ready)) {
		if((fd = socket_accept(socket)) < 0) {
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
					   "accept_socket", versionc,
					   "socket_accept() returned ERROR, exiting");
			exit(1);
		} else	
			if(fcntl(fd, F_SETFL, O_NDELAY) < 0) {
				bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
						   "accept_socket", versionc,
						   "fcntl() returned ERROR, exiting");
				exit(1);
#if 0
				close(fd);
				close(socket);
				fd = -1;
#endif
			}
	} else
		return ERROR;

	return fd;
}
