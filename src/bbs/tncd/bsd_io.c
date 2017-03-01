#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef SUNOS
#include <termio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif
#include <fcntl.h>
#include <sys/time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bsd.h"
#include "tnc.h"
#include "version.h"

extern int bbsd_sock;
struct Tnc tnc[MAX_TNC];
extern char versionc[80];
extern char *Bbs_Call;

int
asy_init(int dev, char *ttydev)
{
	extern int ReceiveSocket, SendSocket;
	struct termios tt;

	if(ReceiveSocket) {
		int sock;
		fd_set ready;
		int fdlimit;

		if((sock = socket_listen(&ReceiveSocket)) == ERROR) {
			error_log("tncd: ReceiveSocket open failed on %d", ReceiveSocket);
			error_print_exit(1);
		}

		FD_ZERO(&ready);
		FD_SET(sock, &ready);
		fdlimit = sock;
		FD_SET(bbsd_sock, &ready);
		if(bbsd_sock > fdlimit)
			fdlimit = bbsd_sock;
		fdlimit++;

		switch(select(fdlimit, &ready, NULL, NULL, NULL)) {
		case ERROR:
		case 0:
			if(dbug_level & dbgVERBOSE)
				printf("error in select\n");
			exit(2);
		default:
			if(FD_ISSET(bbsd_sock, &ready))
				exit(2);

			if((tnc[dev].fd = socket_accept(sock)) == ERROR) {
				error_log("tncd: socket_accept failed on %d", ReceiveSocket);
				error_print_exit(1);
			}
		}		

		if(dbug_level & dbgVERBOSE)
			printf("tncd: server side connected\n");
		tnc[dev].inuse = TRUE;
		return OK;
	}

	if(SendSocket) {
		while((tnc[dev].fd = socket_open(NULL, SendSocket)) == ERROR)
			sleep(2);
		
		if(dbug_level & dbgVERBOSE)
			printf("tncd: client side connected\n");
		tnc[dev].inuse = TRUE;
		return OK;
	}

	strcpy(tnc[dev].tty, ttydev);

	if((tnc[dev].fd = open(ttydev, (O_RDWR), 0)) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: Could not open device tnc_fd");
		logd_stamp("tncd", "asy_init: bad open");
		exit(1);
	}
 			/* get the stty structure and save it */

#ifdef SUNOS
	if(ioctl(tnc[dev].fd, TCGETS, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: ioctl failed on device");
		return ERROR;
	}
#else
#ifdef TCGETATTR
	if(ioctl(tnc[dev].fd, TCGETATTR, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: ioctl failed on device");
		return ERROR;
	}
#else
	if(ioctl(tnc[dev].fd, TCGETA, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: ioctl failed on device");
		return ERROR;
	}
#endif
#endif

	tt.c_iflag = 0;
	tt.c_oflag = 0;
	tt.c_cflag = B4800 | CS8 | CREAD;
	tt.c_lflag = 0;


#ifdef SUNOS
	if(ioctl(tnc[dev].fd, TCSETS, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETA)");
		exit(1);
	}
#else
#ifdef TCSETATTR
	if(ioctl(tnc[dev].fd, TCSETATTR, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETATTR)");
		exit(1);
	}
#else
	if(ioctl(tnc[dev].fd, TCSETA, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETA)");
		exit(1);
	}
#endif
#endif

	tnc[dev].inuse = TRUE;
	return OK;
}

/* Send a buffer to serial transmitter */
int
asy_output(int dev, char *buf, int cnt)
{
	if(tnc[dev].inuse == FALSE)
		return ERROR;

	if(write(tnc[dev].fd, buf, cnt) < cnt) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_output");
		logd_stamp("tncd", "asy_output: bad write, wrong count");
		exit(1);
	}
	return OK;
}


/* Receive characters from asynch line
 * Returns count of characters read
 */

int
asy_recv(int dev, char *buf, int cnt)
{
	fd_set ready;
	struct timeval timeout;
	int fdlimit;
	int result;

	FD_ZERO(&ready);
	FD_SET(tnc[dev].fd, &ready);
	fdlimit = tnc[dev].fd;
	FD_SET(bbsd_sock, &ready);
	if(bbsd_sock > fdlimit)
		fdlimit = bbsd_sock;
	fdlimit++;
	timeout.tv_sec = 0;
	timeout.tv_usec = 35;

	result = select(fdlimit, (void*)&ready, 0, 0, &timeout);

	if(result < 0) {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "asy_recv", versionc,
				   "select call returned ERROR, exiting");
		exit(1);
	}

	if(result == 0)
		return 0;

	if(FD_ISSET(bbsd_sock, &ready))
		exit(2);

	if(FD_ISSET(tnc[dev].fd, &ready)) {
		result = read(tnc[dev].fd, buf, cnt);
		if(result < 0) {
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
					   "asy_recv", versionc,
					   "Error on read, exiting");
			exit(1);
		}

		if(result == 0) {
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
					   "asy_recv", versionc,
					   "read count is 0, why were we selected? exiting");
			exit(1);
		}
		return result;
	}

	bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
			   "asy_recv", versionc,
			   "Hit on select but no match on tnc[?].fd, exiting");
	exit(1);
}
