#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#include "c_cmmn.h"
#include "bsd.h"
#include "monitor.h"
#include "socket.h"
#include "timer.h"
#include "ax25.h"
#include "slip.h"
#include "ax_mbx.h"
#include "tools.h"
#undef YES
#undef NO
#include "bbslib.h"
#include "alib.h"

struct Monitor_Session {
	struct Monitor_Session *next;
	char buf[1024];
	size_t cnt;
	int fd;
	int disp;
	alEventHandle ev;
#define DispNONE	0
#define DispALL		1
#define DispME		2
};

int monitor = ERROR;
static int monitor_socket = ERROR;
static alEventHandle monitor_event_handle;

extern char versionm[80];

struct Monitor_Session *MonProcs = NULL;
char *helpmsg[] = {
	"All ........... monitor all traffic\n",
	"Me ............ monitor just our traffic\n",
	"Off ........... disable monitor\n",
	"STatus ........ show status of all procs\n",
	"SEt ........... set tnc parameters\n",
	"   T1 30\n",
	"   T2 3\n",
	"   T3 300\n",
	"   MAXFRAME 7\n",
	"   PACLEN 220\n",
	"   PTHRESH 110\n",
	"   N2 10\n",
	"   TX [0|1]\n",
	NULL };

static void monitor_control_accept(void *obj, void *arg0, int arg1);
static void monitor_disc(struct Monitor_Session *mp);
static void monitor_read_callback(void *obj, void *arg0, int arg1);
static void monitor_command(struct Monitor_Session *mp, char *buf);
static int monitor_printf(struct Monitor_Session **mp, const char *fmt, ...);
static void monitor_dump(struct Monitor_Session *mp);

int
monitor_init(char *m_bindaddr, int m_port)
{
	alCallback cb;

	if((monitor_socket = socket_listen(m_bindaddr, &m_port)) < 0) {
		log_error(
			"init_ax_control: Problem opening monitor socket "
			"... aborting");
		return ERROR;
	}

	AL_CALLBACK(&cb, NULL, monitor_control_accept);
	if (alEvent_registerFd(monitor_socket, ALFD_READ, cb,
		&monitor_event_handle) != 0)
	{
		log_error("problem registering monitor socket");
		return ERROR;
	}

	return OK;
}

int
monitor_shutdown(void)
{
	if (monitor_event_handle != NULL) {
		alEvent_deregister(monitor_event_handle);
		monitor_event_handle = NULL;
	}
	if (monitor_socket != ERROR) {
		close(monitor_socket);
		monitor_socket = ERROR;
	}

	return 0;
}

int
monitor_enabled(void)
{
	if(MonProcs != NULL)
		return TRUE;
	return FALSE;
}

static void
monitor_disc(struct Monitor_Session *mp)
{
	struct Monitor_Session *tmp = mp;

	if(mp == MonProcs) {
		MonProcs = mp->next;
		mp = MonProcs;
	} else
		mp = mp->next;

	if (tmp->ev != NULL) {
		alEvent_deregister(tmp->ev);
		tmp->ev = NULL;
	}
	close(tmp->fd);
	free(tmp);
}

void
monitor_write(char *s, int me)
{
	struct Monitor_Session *mp;

	mp = MonProcs;
	while(mp) {
		if(mp->disp == DispALL || (mp->disp == DispME && me))
			if(write(mp->fd, s, strlen(s)) < 0) {
				struct Monitor_Session *tmp = mp->next;
				monitor_disc(mp);
				mp = tmp;
				continue;
			}
		mp = mp->next;
	}
}

static void
monitor_control_accept(void *obj, void *arg0, int arg1)
{
	int fd, res;
	alCallback cb;

	if((fd = accept_socket(monitor_socket)) == ERROR) {
		log_error("monitor accept() error");
		exit(1);
	}

	struct Monitor_Session *mp = malloc_struct(Monitor_Session);
	mp->next = NULL;
	mp->fd = fd;
	mp->cnt = 0;

	AL_CALLBACK(&cb, mp, monitor_read_callback);
	res = alEvent_registerFd(mp->fd, ALFD_READ, cb, &mp->ev);
	if (res != 0) {
		log_error("monitor register error");
		exit(1);
	}

	mp->disp = DispALL;

	socket_write(fd, versionm);

	if(MonProcs == NULL)
		MonProcs = mp;
	else {
		struct Monitor_Session *tmp = MonProcs;
		while(tmp->next)
			tmp = tmp->next;
		tmp->next = mp;
	}
}

static void
monitor_read_callback(void *obj, void *arg0, int arg1)
{
	struct Monitor_Session *mp = obj;
	size_t remain;
	ssize_t cnt;
	char *p;

	remain = sizeof(mp->buf) - mp->cnt;
	if (remain == 0) {
		/*
		 * Input has gotten too big without a newline.
		 * Just shut it down.
		 */
		monitor_disc(mp);
		return;
	}

	cnt = read(mp->fd, &mp->buf[mp->cnt], remain);

	if (cnt <= 0) {
		/*
		 * Let's shut down  the process.
		 */
#ifdef TRACE_DBG
		log_debug("monitor_chk: broken pipe");
#endif
		monitor_disc(mp);
		return;
	}

	mp->cnt += cnt;

	while ((p = (char*) memchr(mp->buf, '\n', mp->cnt)) != NULL) {
		*p = 0;
		uppercase(mp->buf);
		monitor_command(mp, mp->buf);
		size_t consumed = (p - mp->buf) + 1;
		memmove(mp->buf, p+1, mp->cnt - consumed);
		mp->cnt -= consumed;
	}
}

static void
monitor_command(struct Monitor_Session *mp, char *buf)
{
	switch(buf[0]) {
	case '?': {
		int i;

		for (i = 0; helpmsg[i] != NULL; i++) {
			if(write(mp->fd, helpmsg[i], strlen(helpmsg[i])) < 0) {
				monitor_disc(mp);
				return;
			}
		}
	}
		break;
	case 'S':
		if(buf[1] == 'E') {
			char *p = buf;
			char *q;
			int value;
			NextSpace(p);
			NextChar(p);

			if(*p == 0)
				break;
			q = get_string(&p);
			if(*p == 0)
				break;
			value = get_number(&p);

			if(!strcmp(q, "T1")) {
				Tncd_T1init = value;
				break;
			}
			if(!strcmp(q, "T2")) {
				Tncd_T2init = value;
				break;
			}
			if(!strcmp(q, "T3")) {
				Tncd_T3init = value;
				break;
			}
			if(!strcmp(q, "MAXFRAME")) {
				if(value > 0 && value < 8)
					Tncd_Maxframe = value;
				break;
			}
			if(!strcmp(q, "PACLEN")) {
				Tncd_Paclen = value;
				break;
			}
			if(!strcmp(q, "PTHRESH")) {
				Tncd_Pthresh = value;
				break;
			}
			if(!strcmp(q, "N2")) {
				Tncd_N2 = value;
				break;
			}
			if (!strcmp(q, "TX")) {
				Tncd_TX_Enable(value ? 1 : 0);
				break;
			}
		} else {
			monitor_dump(mp);
		}

	case 'A':
		mp->disp = DispALL;
		break;
	case 'M':
		mp->disp = DispME;
		break;
	case 'O':
		mp->disp = DispNONE;
		break;
	}
}

static void
monitor_dump(struct Monitor_Session *mp)
{
	struct mboxsess *mpb;
	struct Monitor_Session *mpp = mp;

	monitor_printf(&mpp,
		"host = %s\ncontrol = %d\nmonitor = %d\ndevice = %s\n",
		Tncd_Host, Tncd_Control_Port, Tncd_Monitor_Port,
		Tncd_Device);
	monitor_printf(&mpp,
		"t1/2/3 = %d %d %d\n",
		Tncd_T1init, Tncd_T2init, Tncd_T3init);
	monitor_printf(&mpp,
		"maxframe = %d\npaclen = %d\npthresh = %d\2 = %d\nflags = %d\n",
		Tncd_Maxframe, Tncd_Paclen, Tncd_Pthresh, Tncd_N2,
		Tncd_SLIP_Flags);
	monitor_printf(&mpp,
		"transmit_enabled = %d\n",
		Is_Tncd_TX_Enabled());

	if (mpp == NULL)
		return;

	LIST_FOREACH(mpb, &base, listEntry) {
		char *state;

		if (mpp == NULL)
			return;

		monitor_printf(&mpp,
			"pid=%d spawned=%s fd=%d\n",
			mpb->pid, mpb->spawned ? "TRUE":"FALSE", mpb->fd);

		switch(mpb->axbbscb->state) {
		case DISCONNECTED:	state = "DISCONNECTED"; break;
		case SETUP:			state = "SETUP"; break;
		case DISCPENDING:	state = "DISCPENDING"; break;
		case CONNECTED:		state = "CONNECTED"; break;
		case RECOVERY:		state = "RECOVERY"; break;
		case FRAMEREJECT:	state = "FRAMEREJECT"; break;
		default:			state = "Unknown"; break;
		}

		monitor_printf(&mpp,
			"vs=%d vr=%d unack=%d retries=%d %s\n",
			mpb->axbbscb->vs, mpb->axbbscb->vr,
			mpb->axbbscb->unack, mpb->axbbscb->retries,
			state);
	}
}

static int
monitor_printf(struct Monitor_Session **mp, const char *fmt, ...)
{
	char obuf[256];
	int nwritten;
	size_t len;

	if (*mp == NULL)
		return -1;

	va_list ap;
	va_start(ap, fmt);
	nwritten = vsnprintf(obuf, sizeof(obuf)-1, fmt, ap);
	va_end(ap);
	
	if (nwritten < 0)
		goto MpError;

	len = strlen(obuf);

	if (write((*mp)->fd, obuf, len) < len)
		goto MpError;

	return 0;

MpError:
	monitor_disc(*mp);
	*mp = NULL;
	return -1;
}
