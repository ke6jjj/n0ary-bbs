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
	int fd = -1;
	int result;

	if ((fd = socket_accept(socket)) < 0) {
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

	return fd;
}
