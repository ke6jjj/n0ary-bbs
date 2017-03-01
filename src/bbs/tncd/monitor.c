#include <stdio.h>
#include <errno.h>

#include "c_cmmn.h"
#include "monitor.h"
#include "socket.h"
#include "timer.h"
#include "ax25.h"
#include "ax_mbx.h"
#include "tools.h"
#undef YES
#undef NO
#include "bbslib.h"

int monitor = ERROR;
int monitor_socket = ERROR;

extern char versionm[80];

struct Monitor_Procs *MonProcs = NULL;
char *helpmsg[] = {
	"ALL ........... monitor all traffic\n",
	"ME ............ monitor just our traffic\n",
	"OFF ........... disable monitor\n",
	"STATUS ........ show status of all procs\n",
	"SET ........... set tnc parameters\n",
	"   T1 30\n",
	"   T2 4\n",
	"   T3 300\n",
	"   MAXFRAME 7\n",
	"   PACLEN 220\n",
	"   N2 10\n",
	NULL };

int
monitor_enabled(void)
{
	if(MonProcs != NULL)
		return TRUE;
	return FALSE;
}

struct Monitor_Procs *
monitor_disc(struct Monitor_Procs *mp)
{
	struct Monitor_Procs *tmp = mp;

	if(mp == MonProcs) {
		MonProcs = mp->next;
		mp = MonProcs;
	} else
		mp = mp->next;

	close(tmp->fd);
	free(tmp);
	return mp;
}

void
monitor_write(char *s, int me)
{
	struct Monitor_Procs *mp;

	monitor_chk();

	mp = MonProcs;
	while(mp) {
		if(mp->disp == DispALL || (mp->disp == DispME && me))
			if(write(mp->fd, s, strlen(s)) < 0) {
				mp = monitor_disc(mp);
				continue;
			}
		mp = mp->next;
	}
}

void
monitor_control(void)
{
	int fd;

	if((fd = accept_socket(monitor_socket)) != ERROR) {
		struct Monitor_Procs *mp = malloc_struct(Monitor_Procs);
		mp->next = NULL;
		mp->fd = fd;
		mp->disp = DispALL;

		socket_write(fd, versionm);

		if(MonProcs == NULL)
			MonProcs = mp;
		else {
			struct Monitor_Procs *tmp = MonProcs;
			while(tmp->next)
				tmp = tmp->next;
			tmp->next = mp;
		}
	}
	monitor_chk();
}

void
monitor_chk(void)
{
		/* looking for a disconnect */
	char buf[1024];
	struct Monitor_Procs *mp = MonProcs;

	while(mp) {
		int cnt = read(mp->fd, buf, 1024);
		char *p;

		switch(cnt) {
		case 0:
#ifdef TRACE_DBG
fprintf(stderr, "monitor_chk: broken pipe\n"); fflush(stderr);
#endif
			mp = monitor_disc(mp);
			continue;

		case ERROR:
			switch(errno) {
			case EAGAIN:
#if EWOULDBLOCK != EAGAIN
			case EWOULDBLOCK:
#endif
				break;
			default:
#ifdef TRACE_DBG
fprintf(stderr, "monitor_chk: broken pipe\n"); fflush(stderr);
#endif
						/* anything other than the items above indicate a
						 * fatal problem or broken pipe. Let's shut down
						 * the process.
						 */
				mp = monitor_disc(mp);
				continue;
			}
			break;
		default:
			if((p = (char*)index(buf, '\r')) != NULL)
				*p = 0;
			uppercase(buf);
			switch(buf[0]) {
			case '?':
				{
					int i = 0;
					int bad = FALSE;

					while(helpmsg[i] != NULL) {
						if(write(mp->fd, helpmsg[i], strlen(helpmsg[i])) < 0) {
							mp = monitor_disc(mp);
							bad = TRUE;
							break;
						}
						i++;
					}
					if(bad)
						continue;
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
						Tncd_Paclen = Tncd_Pthresh = value;
						break;
					}
					if(!strcmp(q, "N2")) {
						Tncd_N2 = value;
						break;
					}
				} else {
					struct mboxsess *mpb = base;
					char obuf[256];
					int bad = FALSE;

					sprintf(obuf, 
						"host = %s\ncontrol = %d\nmonitor = %d\ndevice = %s\n",
						Tncd_Host, Tncd_Control_Port, Tncd_Monitor_Port,
						Tncd_Device);
					if(write(mp->fd, obuf, strlen(obuf)) < 0) {
						mp = monitor_disc(mp);
						continue;
					}
					sprintf(obuf, "t1/2/3 = %d %d %d\n",
						Tncd_T1init, Tncd_T2init, Tncd_T3init);
					if(write(mp->fd, obuf, strlen(obuf)) < 0) {
						mp = monitor_disc(mp);
						continue;
					}
					sprintf(obuf, "maxframe = %d\npaclen = %d\n2 = %d\n",
						Tncd_Maxframe, Tncd_Paclen, Tncd_N2);
					if(write(mp->fd, obuf, strlen(obuf)) < 0) {
						mp = monitor_disc(mp);
						continue;
					}


					while(mpb != NULLMBS) {
						char *state;

						sprintf(obuf,
							"pid=%d spawned=%s fd=%d socket=%d port=%d\n",
							mpb->pid, mpb->spawned ? "TRUE":"FALSE", mpb->fd,
							mpb->socket, mpb->port);
						if(write(mp->fd, obuf, strlen(obuf)) < 0) {
							mp = monitor_disc(mp);
							bad = TRUE;
							break;
						}
						switch(mpb->axbbscb->state) {
						case DISCONNECTED:	state = "DISCONNECTED"; break;
						case SETUP:			state = "SETUP"; break;
						case DISCPENDING:	state = "DISCPENDING"; break;
						case CONNECTED:		state = "CONNECTED"; break;
						case RECOVERY:		state = "RECOVERY"; break;
						case FRAMEREJECT:	state = "FRAMEREJECT"; break;
						default:			state = "Unknown"; break;
						}

						sprintf(obuf,
							"vs=%d vr=%d unack=%d retries=%d %s\n",
							mpb->axbbscb->vs, mpb->axbbscb->vr,
							mpb->axbbscb->unack, mpb->axbbscb->retries,
							state);
							if(write(mp->fd, obuf, strlen(obuf)) < 0) {
								mp = monitor_disc(mp);
								bad = TRUE;
								break;
							}

						NEXT(mpb);
					}
					if(bad)
						continue;
				}
				break;
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
		mp = mp->next;
	}
}

