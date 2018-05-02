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

extern char versionc[80];
extern char *Bbs_Call;

struct asy {
	int fd;

	/* Outgoing packet queue */
	struct mbuf *sndq;
	struct mbuf *tbp;
	int sndcnt;

	/* Upcall for received data */
	buf_recv_fn recv;
	void *recv_arg;

	/* Whom to notify if there are errors */
	asy_notify_fn notify;
	void *notify_arg;

	uint8_t tx_enabled; /* Whether to accept outgoing data */

	alEventHandle ev_handle; /* Event system handle for fd */
	alCallback work_cb;  /* Callback handle to self for dequeue task */
};

static asy *asy_init_common(void);
static int  asy_init_serial(asy *, char *ttydev);
static int  asy_init_tcp(asy *, char *host_port_spec);
static void asy_read_callback(void *obj, void *arg0, int arg1);
static int  asy_recv(asy *asy, char *buf, int cnt);
static void asy_work_callback(void *obj, void *arg0, int arg1);

static void default_notify(void *ctx, asy *asy, int is_read, int error);

struct asy *
asy_init(char *devspec)
{
	asy *asy = asy_init_common();
	if (asy == NULL)
		return NULL;

	if (strchr(devspec, ':') != NULL) {
		/* TNC is on a terminal server */
		if (asy_init_tcp(asy, devspec) == 0)
			return asy;
	} else {
		/* TNC is a local device */
		if (asy_init_serial(asy, devspec) == 0)
			return asy;
	}

	free(asy);
	return NULL;
}

struct asy *
asy_init_from_fd(int fd)
{
	struct asy *asy = asy_init_common();
	if (asy == NULL)
		return NULL;

	asy->fd = fd;

	return asy;
}

int
asy_set_recv(asy *asy, buf_recv_fn recv, void *arg)
{
	asy->recv = recv;
	asy->recv_arg = arg;
	return 0;
}

int
asy_set_notify(asy *asy, asy_notify_fn fn, void *arg)
{
	asy->notify = fn;
	asy->notify_arg = arg;
	return 0;
}

int
asy_enable(asy *asy, int enable)
{
	asy->tx_enabled = enable;
	return 0;
}

int
asy_enabled(asy *asy)
{
	return asy->tx_enabled;
}

int
asy_deinit(asy *asy)
{
	if (asy->tbp != NULLBUF)
		free_p(asy->tbp);

	if (asy->sndq != NULL)
		free_q(&asy->sndq);

	free(asy);

	return 0;
}

int
asy_start(asy *asy)
{
	alCallback cb;
	int res;

	if (asy->recv == NULL)
		/* There's no one set up to receive data! */
		return -1;

	if (asy->ev_handle != NULL)
		/* Already started! */
		return -1;

	assert(asy->fd != -1);

	AL_CALLBACK(&asy->work_cb, (void*) asy, asy_work_callback);
	AL_CALLBACK(&cb, (void*) asy, asy_read_callback);
	res = alEvent_registerFd(asy->fd, ALFD_READ, cb, &asy->ev_handle);

	return res;
}

/* Put a packet on the output queue, and if necessary, start the
 * output dequeue routine.
 */
int
asy_send(void *asyp, struct mbuf *data)
{
	asy *asy = (struct asy *) asyp;

	enqueue(&asy->sndq, data);
	asy->sndcnt++;

	if(asy->tbp == NULLBUF)
		alEvent_queueCallback(asy->work_cb, ALCB_UNIQUE, asy, 0);

	return 0;
}

int
asy_stop(asy *asy)
{
	if (asy->ev_handle != NULL)
		alEvent_deregister(asy->ev_handle);

	alEvent_killCallbacks(asy);

	return 0;
}

static asy *
asy_init_common(void)
{
	struct asy *asy = malloc_struct(asy);

	if (asy != NULL) {
		asy->sndq = NULL;
		asy->sndcnt = 0;
		asy->tbp = NULL;
		asy->recv = NULL;
		asy->recv_arg = NULL;
		asy->notify = default_notify;
		asy->notify_arg = NULL;
		asy->tx_enabled = FALSE;
	}

	return asy;
}

static int
asy_init_serial(struct asy *asy, char *ttydev)
{
	struct termios tt;

	if((asy->fd = open(ttydev, (O_RDWR), 0)) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: Could not open device tnc_fd");
		logd_stamp("tncd", "asy_init: bad open");
		exit(1);
	}
 	/* get the stty structure and save it */

#ifdef HAVE_TERMIOS
	if (tcgetattr(asy->fd, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: tcgetattr failed on device");
		return ERROR;
	}
#else
#ifdef SUNOS
	if(ioctl(asy->fd, TCGETS, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: ioctl failed on device");
		return ERROR;
	}
#else
#ifdef TCGETATTR
	if(ioctl(asy->fd, TCGETATTR, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror ("asy_init: ioctl failed on device");
		return ERROR;
	}
#else
	if(ioctl(asy->fd, TCGETA, &tt) < 0) {
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
	if (tcsetattr(asy->fd, TCSANOW, &tt) < 0) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad tcsetattr");
		exit(1);
	}
#else
#  ifdef SUNOS
	if(ioctl(asy->fd, TCSETS, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETA)");
		exit(1);
	}
#  else
#    ifdef TCSETATTR
	if(ioctl(asy->fd, TCSETATTR, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETATTR)");
		exit(1);
	}
#    else
	if(ioctl(asy->fd, TCSETA, &tt)) {
		if(dbug_level & dbgVERBOSE)
			perror("asy_init: could not set serial parameters");
		logd_stamp("tncd", "asy_init: bad ioctl (TCSETA)");
		exit(1);
	}
#    endif
#  endif
#endif

	return OK;
}

static int
asy_init_tcp(struct asy *asy, char *host_spec)
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

	asy->fd = fd;

	return OK;
}

/* Receive characters from asynch line
 * Returns count of characters read
 */
static int
asy_recv(asy *asy, char *buf, int cnt)
{
	int result;

	result = read(asy->fd, buf, cnt);
	if(result < 0 && (errno != EWOULDBLOCK && errno != EAGAIN))
		asy->notify(asy->notify_arg, asy, 1, errno);
	else if(result == 0)
		asy->notify(asy->notify_arg, asy, 1, 0);

	return result;
}

/* Send a buffer to serial transmitter */
static int
asy_output(struct asy *asy, char *buf, int cnt)
{
	if (!asy->tx_enabled)
		return OK;

	if (write(asy->fd, buf, cnt) < cnt)
		return -1;

	return OK;
}

static void
default_notify(void *ctx, asy *asy, int is_read, int error)
{
	if (is_read) {
		if (error != 0) {
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
					   "asy_recv", versionc,
					   "Error on read, exiting");
			exit(1);
		} else {
			logd_stamp("tncd", "asy_: End-of-file");
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   	"asy_recv", versionc,
				   	"End-of-file reading TNC, exiting");
			exit(1);
		}
	} else {
		if(dbug_level & dbgVERBOSE)
			perror("asy_output");
		logd_stamp("tncd", "asy_output: bad write, wrong count");
		exit(1);
	}
}

static void
asy_read_callback(void *obj, void *arg0, int arg1)
{
	struct asy *asy = (struct asy *) obj;
	char buf[256];
	int count;

	assert(asy->fd != -1 && asy->ev_handle != NULL);
	assert(asy->recv != NULL);

	/* Process _some_ pending input */
	count = asy_recv(asy, buf, sizeof(buf));
	if (count > 0)
		asy->recv(asy->recv_arg, buf, count);
}

/* Check outgoing queue for work */
static void
asy_work_callback(void *obj, void *arg0, int arg1)
{
	struct asy *asy = (struct asy *) obj;
	int res;

	if(asy->tbp != NULLBUF){
		/* transmission just completed */
		free_p(asy->tbp);
		asy->tbp = NULLBUF;
	}
	if(asy->sndq == NULLBUF)
		return;	/* No work */

	asy->tbp = dequeue(&asy->sndq);
	asy->sndcnt--;

	res = asy_output(asy, asy->tbp->data, asy->tbp->cnt);

	if (res != 0) {
		asy->notify(asy->notify_arg, asy, 0, res);
		return;
	}

	/* Keep calling back until queue is empty */
	alEvent_queueCallback(asy->work_cb, ALCB_UNIQUE, asy, 0);
}


