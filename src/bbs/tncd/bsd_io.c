#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#ifdef SUNOS
#include <termio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif
#include <fcntl.h>
#include <sys/time.h>

#include "alib.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bsd.h"
#include "tnc.h"
#include "version.h"

extern int bbsd_sock;
struct Tnc tnc[MAX_TNC];
int Tncd_TX_Enabled;
extern char versionc[80];
extern char *Bbs_Call;

static int asy_init_serial(int dev, char *ttydev);
static int asy_init_tcp(int dev, char *host_port_spec);
static void asy_read_callback(void *obj, void *arg0, int arg1);

int
asy_init(int dev, char *ttydev)
{
	strcpy(tnc[dev].tty, ttydev);
	tnc[dev].read_cb = NULL;
	tnc[dev].read_arg = NULL;

	if (strchr(ttydev, ':') != NULL) {
		/* TNC is on a terminal server */
		return asy_init_tcp(dev, ttydev);
	} else {
		/* TNC is a local device */
		return asy_init_serial(dev, ttydev);
	}
}

int
asy_set_read_cb(int dev, void (*cbf)(int, void *), void *arg)
{
	alCallback cb;
	int res;

	tnc[dev].read_cb = cbf;
	tnc[dev].read_arg = arg;

	if (tnc[dev].evHandle != NULL)
		alEvent_deregister(tnc[dev].evHandle);

	if (cbf != NULL) {
		assert(tnc[dev].fd != -1);
		AL_CALLBACK(&cb, (void*) dev, asy_read_callback);
		res = alEvent_registerFd(tnc[dev].fd, ALFD_READ,
			cb, &tnc[dev].evHandle);
	} else {
		res = 0;
	}

	return res;
}

static int
asy_init_serial(int dev, char *ttydev)
{
	struct termios tt;

	if((tnc[dev].fd = open(ttydev, (O_RDWR), 0)) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: Could not open device tnc_fd");
		logd_stamp("tncd", "asy_init: bad open");
		exit(1);
	}
 	/* get the stty structure and save it */

#ifdef HAVE_TERMIOS
	if (tcgetattr(tnc[dev].fd, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: tcgetattr failed on device");
		return ERROR;
	}
#else
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
#endif

	tt.c_iflag = 0;
	tt.c_oflag = 0;
	tt.c_cflag = B4800 | CS8 | CREAD;
	tt.c_lflag = 0;


#ifdef HAVE_TERMIOS
	if (tcsetattr(tnc[dev].fd, TCSANOW, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad tcsetattr");
		exit(1);
	}
#else
#  ifdef SUNOS
	if(ioctl(tnc[dev].fd, TCSETS, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETA)");
		exit(1);
	}
#  else
#    ifdef TCSETATTR
	if(ioctl(tnc[dev].fd, TCSETATTR, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETATTR)");
		exit(1);
	}
#    else
	if(ioctl(tnc[dev].fd, TCSETA, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETA)");
		exit(1);
	}
#    endif
#  endif
#endif

	tnc[dev].inuse = TRUE;
	return OK;
}

static int
asy_init_tcp(int dev, char *host_spec)
{
	char *colon, *host, *mutable;
	int port, fd;
	
	mutable = strdup(host_spec);
	colon = strchr(mutable, ':');

	if (colon == NULL) {
		free(mutable);
		logd_stamp("tncd", "asy_init: bad host spec");
		exit(1);
	}

	*colon = '\0';
	host = mutable;
	port = atoi(colon + 1);

	if ((fd = socket_open(host, port)) < 0) {
		free(mutable);
		logd_stamp("tncd", "asy_init: can't connect to host");
		exit(1);
	}

	free(mutable);

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		logd_stamp("tncd", "asy_init: can't O_NONBLOCK");
		close(fd);
		exit(1);
	}

	tnc[dev].fd = fd;
	tnc[dev].inuse = TRUE;

	return OK;
}

/* Send a buffer to serial transmitter */
int
asy_output(int dev, char *buf, int cnt)
{
	if(tnc[dev].inuse == FALSE)
		return ERROR;

	if (Tncd_TX_Enabled != 1)
		return OK;

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
	int result;

	result = read(tnc[dev].fd, buf, cnt);
	if(result < 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "asy_recv", versionc,
				   "Error on read, exiting");
		exit(1);
	}

	if(result == 0) {
		logd_stamp("tncd", "asy_: End-of-file");
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "asy_recv", versionc,
				   "End-of-file reading TNC, exiting");
		exit(1);
	}

	return result;
}

static void
asy_read_callback(void *obj, void *arg0, int arg1)
{
	int dev = (int) obj;

	assert(dev < MAX_TNC);
	assert(tnc[dev].fd != -1 && tnc[dev].evHandle != NULL);
	assert(tnc[dev].read_cb != NULL);

	tnc[dev].read_cb(dev, tnc[dev].read_arg);
}
