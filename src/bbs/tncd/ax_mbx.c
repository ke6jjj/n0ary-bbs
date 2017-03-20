#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "alib.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "global.h"
#include "timer.h"
#include "ax25.h"
#include "mbuf.h"
#include "socket.h"

#if 0
#define DEBUG1 1
#endif

extern char
    versionc[80],
	*Bbs_My_Call,
	*Bin_Dir,
	*Bbs_Dir;

#include "ax_mbx.h"

static void ax_control_accept(void *obj, void *arg0, int arg1);
static void mbx_handle_network(void *obj, void *arg0, int arg1);
static void mbx_handle_child_exit(void *obj, void *arg0, int arg1);
static void mbx_notify_sendable(struct mboxsess *mbp, size_t amount, int q);
static void mbx_sendable_changed(void *obj, void *arg0, int arg1);
static void mbx_process_tx_queue(struct mboxsess *mbp);
static void mbx_nagle_timer(void *obj, void *arg0, int arg1);

static void
	convrt2cr(char *data, int cnt),
	convrt2nl(char *data, int cnt),
	mbx_tx(struct ax25_cb *axp, int cnt),
	mbx_rx(struct ax25_cb *axp, int cnt),
	mbx_accept(void *, void *, int),
	mbx_pid_exited(void *, void *, int),
	shutdown_process(struct mboxsess *mbp, int issue_disc),
	set_ax25_params(char *p, struct ax25_cb *axp),
	freembox(struct mboxsess *mbp),
	initmbox(struct mboxsess *mbp);

static int
	calleq(struct ax25_cb *axp, struct ax25_addr *addr),
	command(struct mboxsess *mbp);

static struct mboxsess
	*newmbox(void);

struct mboxsess *base = NULLMBS;  /*pointer to base of mailbox chain*/
struct mboxsess fwdstruct;	/*forwarding session*/
static struct mboxsess *newmbox();

int control = 0;
char *Tncd_Host;
char *Tncd_Name;
char *Tncd_Device;

extern struct ax25_addr bbscall, fwdcall;

static int cntrl_socket;
static alEventHandle al_cntrl_handle;

/* We need to setup a socket that we will check from time to time to
 * see if a program would like access to the tnc's or to status. This
 * is the mechanism for polling for weather or forwarding.
 */

int
ax_control_init(char *c_bindaddr, int c_port)
{
	alCallback cb;

	int port = c_port;
	if((cntrl_socket = socket_listen(c_bindaddr, &port)) < 0) {
		fprintf(stderr,
			"ax_control_init: Problem opening control socket "
			"... aborting\n");
		return ERROR;
	}
	assert(port == c_port);

	AL_CALLBACK(&cb, NULL, ax_control_accept);
	if (alEvent_registerFd(cntrl_socket, ALFD_READ, cb,
		&al_cntrl_handle) != 0)
	{
		fprintf(stderr, "problem registering control socket\n");
		return ERROR;
	}

	return OK;
}

int
ax_control_shutdown(void)
{
	if (al_cntrl_handle != NULL) {
		alEvent_deregister(al_cntrl_handle);
		al_cntrl_handle = NULL;
	}
	if (cntrl_socket != ERROR) {
		close(cntrl_socket);
		cntrl_socket = ERROR;
	}

	return 0;
}


/* accept a new control port connection (processes can ask questions regarding
 * current connections, etc.)
 */
static void
ax_control_accept(void *obj, void *arg0, int arg1)
{
	int fd;
	alCallback cb;

	if((fd = accept_socket(cntrl_socket)) != ERROR) {
		struct mboxsess *mbp = newmbox();
		initmbox(mbp);

		if(dbug_level & dbgVERBOSE)
			printf("ax_control(), accepted socket %d\n", fd);
		mbp->fd = fd;

		AL_CALLBACK(&cb, mbp, mbx_handle_network);
		int res = alEvent_registerFd(fd, ALFD_READ, cb,
			&mbp->al_fd_handle);
		if (res != 0) {
			fprintf(stderr, "mbx control register failure\n");
			exit(1);
		}
		
		socket_write(fd, versionc);
	} else {
		fprintf(stderr, "new control connection accept error\n");
		exit(1);
	}
}

/*
 * Accept an inbound control session for an existing mailbox session
 * (generally this is a newly spawned BBS trying to reach back to
 * its spawning AX25 connection).
 */
static void
mbx_accept(void *obj, void *arg0, int arg1)
{
	struct mboxsess *mbp = obj;

	if ((mbp->fd = accept_socket(mbp->socket)) == ERROR ) {
		fprintf(stderr, "mbx_accept() error\n");
		shutdown_process(mbp, TRUE);
	}

	/* We can stop listening for new connections now */
	alEvent_deregister(mbp->al_socket_handle);
	mbp->al_socket_handle = NULL;
	close(mbp->socket);
	mbp->socket = ERROR;

	/* Now start listening for activity on the connection*/
	alCallback cb;
	AL_CALLBACK(&cb, mbp, mbx_handle_network);
	int res = alEvent_registerFd(mbp->fd, ALFD_READ, cb,
		&mbp->al_fd_handle);
	if (res != 0) {
		fprintf(stderr, "mbx network i/o register problem\n");
		exit(1);
	}

	/*
	 * We can also stop listening for child exit events. We'll
	 * receive such notifications as closures on the control
	 * socket.
	 */
	res = alEvent_deregister(mbp->al_proc_handle);
	if (res != 0) {
		fprintf(stderr, "mbx remove proc callback problem\n");
		exit(1);
	}
	mbp->al_proc_handle = NULL;
}


static struct mboxsess *
newmbox(void)
{
	struct mboxsess *mbp ;
#ifdef DEBUG1
	int cnt = 2;
	char buf[80];
#endif

	if(base == NULLMBS){
		base = (struct mboxsess *)malloc(sizeof(struct mboxsess));
		base->next = NULLMBS;
		initmbox(base);
#ifdef DEBUG1
		bbsd_msg("usrcnt=1");
#endif
		return base;
	} else{
		mbp = base;
		while(mbp->next != NULLMBS) {	   /*go up the chain to the top*/
			mbp = mbp->next;
#ifdef DEBUG1
			cnt++;
#endif
		}
		mbp->next = (struct mboxsess *)malloc(sizeof(struct mboxsess));
		mbp->next->next = NULLMBS;
		initmbox(mbp->next);
#ifdef DEBUG1
		sprintf(buf, "usrcnt=%d", cnt);
		bbsd_msg(buf);
#endif
		return mbp->next;
	}
}

static void
initmbox(struct mboxsess *mbp)
{
	mbp->pid = 0;	
	mbp->spawned = FALSE;
	mbp->sendable_count = 0;
	mbp->nagle_timer_id = -1;
	mbp->nagle_gate_down = 1;
	mbp->byte_cnt = 0;
	mbp->cmd_cnt = 0;
	mbp->cmd_state = 1;
	mbp->orig = NULL;
	mbp->fd = ERROR;
	mbp->al_fd_handle = NULL;
	mbp->socket = ERROR;
	mbp->al_socket_handle = NULL;
	mbp->networkfull = 0;
}

static void
freembox(struct mboxsess *mbp)
{
	struct mboxsess *p;
#ifdef DEBUG1
	int cnt = 0;
	char buf[80];
#endif
	
	if(mbp == base){					/*special case for base session*/
		base = base->next;				/*make base point to next session*/
		free(mbp);						/*free up the storage*/
	} else {

		if(mbp->orig)
			free(mbp->orig);

		p = base;
		for(;;){
			if(p->next == mbp) {		/*if next upward session is THE one*/
				p->next = mbp->next;	/*eliminate the next upward session*/
				free(mbp);
				break;
			}
			if(p->next == NULLMBS) {	/*something is wrong here!*/
				free(mbp);				/*try to fix it*/
				break;
			}
			p = p->next;
		}
	}

#ifdef DEBUG1
	p = base;
	while(p != NULLMBS) {	   /*go up the chain to the top*/
		NEXT(p);
		cnt++;
	}
	sprintf(buf, "usrcnt=%d", cnt);
	bbsd_msg(buf);
#endif
}

static void
set_ax25_params(char *p, struct ax25_cb *axp)
{
	char param[20];
	int value;

	NextChar(p);

	if(*p == 0)
		return;
	strcpy(param, get_string(&p));
	if(*p == 0)
		return;
	value = get_number(&p);

	uppercase(param);
	if(!strcmp(param, "T1")) {
		axp->t1.start = value * (axp->addr.ndigis + 1);
		return;
	}
	if(!strcmp(param, "T2")) {
		axp->t2.start = value * (axp->addr.ndigis + 1);
		return;
	}
	if(!strcmp(param, "T3")) {
		axp->t3.start = value * (axp->addr.ndigis + 1);
		return;
	}
	if(!strcmp(param, "MAXFRAME")) {
		axp->maxframe = value;
		return;
	}
	if(!strcmp(param, "PACLEN")) {
		axp->paclen = value;
		axp->pthresh = value;
		return;
	}
	if(!strcmp(param, "N2")) {
		axp->n2 = value;
		return;
	}
}

static int
command(struct mboxsess *mbp)
{
	struct ax25 addr;
	extern int axwindow;
	int i;

	for(i=0; i<mbp->cmd_cnt; i++) {
		char call[10], *q, *p = mbp->command[i];

		if(dbug_level & dbgVERBOSE)
			printf("Command: %s\n", p);

		switch(*p++) {
		case 'B':
			return ERROR;

		case 'S':
			set_ax25_params(p, mbp->axbbscb);
			break;

		case 'C':
			/* N0ARY-1>N6ZFJ,N6UNE,N6ZZZ
			 *
			 *   MYCALL N0ARY-1
			 *   C N6ZFJ via N6UNE,N6ZZZ
			 */

			q = call;
			while(*p != '>') *q++ = *p++;
			*q = 0;
			mbp->orig = (struct ax25_addr *)malloc(sizeof(struct ax25_addr));
#if 1
			setcall(&addr.source, call);
			setcall(mbp->orig, call);
#else
			setcall(&addr.source, Bbs_My_Call);
			setcall(mbp->orig, Bbs_My_Call);
#endif
			p++;

			q = call;
			while(*p != ',' && *p) *q++ = *p++;
			*q = 0;
			setcall(&addr.dest, call);

			addr.ndigis = 0;
			while(*p++ == ',' && addr.ndigis < 8) {
				q = call;
				while(*p != ',' && *p) *q++ = *p++;
				*q = 0;
				setcall(&addr.digis[addr.ndigis], call);
				addr.ndigis++;
			}

			if(find_ax25(&addr.source, &addr.dest) != NULLAX25)
				return ERROR;

			mbp->axbbscb =
				open_ax25(&addr,axwindow,mbx_rx,mbx_tx,mbx_state,0,(char *)0);
		}
	}
	mbp->cmd_cnt = 0;
	return OK;
}

/*
 * Process bytes from a mailbox session's network socket.
 */
static void
mbx_handle_network(void *obj, void *arg0, int arg1)
{
	struct mboxsess * mbp = obj;
	char buf[1025];
	int cnt;

	/* we do have an active bbs or outgoing process. If we don't
	 * have any data pending on this process then check to see
	 * if the process has sent us something new.
	 */
	size_t available = sizeof(mbp->buf) - mbp->byte_cnt;

	if(available > 0) {
		size_t readamt = min(available, (sizeof(buf)-1));
		cnt = read(mbp->fd, buf, readamt);
		if (cnt <= 0) {
			if(dbug_level & dbgVERBOSE)
				printf("axchk: %s broken pipe\n", mbp->call);
			shutdown_process(mbp, TRUE);
			return;
		}
		int cmd_cnt = 0;
		char *p = buf;
		char *q = &mbp->buf[mbp->byte_cnt];

		buf[cnt] = 0;
		if(dbug_level & dbgVERBOSE)
			printf("%s<{%s}\n", mbp->call, buf);

		while(cnt--) {
			if(*p == '\r') {
				p++;
				continue;
			}

			switch(mbp->cmd_state) {
			case 0:	/* IDLE */
				if(*p == '\n')
					mbp->cmd_state++;
				break;
			case 1: /* CR seen looking for '~' */
				if(*p == '~') {
					mbp->cmd_state++;
					p++;
					continue;
				}
				mbp->cmd_state = 0;
				break;
			case 2: /* '~' seen */
				if(*p == '~') {
					mbp->cmd_state = 0;
					break;
				}
				mbp->cmd_state++;
				cmd_cnt = 0;
				mbp->command[mbp->cmd_cnt][cmd_cnt++] = *p++;
				continue;
			case 3: /* in command */
				if(*p == '\n') {
					mbp->command[mbp->cmd_cnt++][cmd_cnt] = 0;
					mbp->cmd_state = 1;
				} else
					mbp->command[mbp->cmd_cnt][cmd_cnt++] = *p;
				p++;
				continue;
			}
			if(*p) {
				*q++ = *p++;
				mbp->byte_cnt++;
			} else
				p++;
		}
	}

	if (mbp->networkfull == 0 && mbp->byte_cnt == sizeof(mbp->buf)) {
		/*
		 * The packetization buffer is full. We need to disable
		 * read events from the network socket until room comes
		 * available.
		 */
		alEvent_setFdEvents(mbp->al_fd_handle, 0);
		mbp->networkfull = 1;
	}

	if(mbp->cmd_cnt) {
		if(command(mbp) == ERROR) {
			shutdown_process(mbp, TRUE);
			return;
		}
	}

	mbx_process_tx_queue(mbp);
}


static void
mbx_process_tx_queue(struct mboxsess *mbp)
{
	struct mbuf *bp;
	char *cp;
	int testsize,size;

	if (mbp->byte_cnt == 0)
		return;

	if (mbp->nagle_timer_id != -1) {
		/*
		 * There's a Nagle timer in effect to keep short, useless
		 * packets from being transmitted. See if it should be
		 * cancelled on account of the amount of data queued.
		 */
		if (mbp->byte_cnt < MBOX_NAGLE_GATE_SIZE) {
			/*
			 * There's not enough data to override the timer,
			 * wait until there's more data or the timer
			 * goes off.
			 */
			return;
		}

		/*
		 * We've got enough data to send now, cancel the
		 * timer.
		 */
		if(dbug_level & dbgVERBOSE)
			printf("axsend: %s lifting Nagle gate due to size\n",
				mbp->call);
		alEvent_cancelTimer(mbp->nagle_timer_id);
		mbp->nagle_timer_id = -1;
		mbp->nagle_gate_down = 0;
	} else if (mbp->byte_cnt < MBOX_NAGLE_GATE_SIZE &&
		mbp->nagle_gate_down)
	{
		/*
		 * We should wait a little bit until sending any
		 * data.
		 */
		if(dbug_level & dbgVERBOSE)
			printf("axsend: %s starting Nagle timer\n",
				mbp->call);
		alCallback cb;
		AL_CALLBACK(&cb, mbp, mbx_nagle_timer);
		mbp->nagle_timer_id = alEvent_addTimer(MBOX_NAGLE_TIMER,
			0, cb);
		return;
	}

	while (mbp->byte_cnt && mbp->sendable_count) {
		if(dbug_level & dbgVERBOSE)
			printf("axsend: %s byte_cnt = %d\n", mbp->call,
				mbp->byte_cnt);
		testsize = min(mbp->sendable_count, mbp->axbbscb->paclen+1);
		size = min(testsize, mbp->byte_cnt) + 1;
		bp = alloc_mbuf(size);			 
		bp->cnt = size;

		cp = bp->data;
		*cp++ = PID_FIRST | PID_LAST | PID_NO_L3;
		size--;
	
		memcpy(cp, mbp->buf, size);
		mbp->byte_cnt -= size;
		mbp->sendable_count -= size;

		if (mbp->byte_cnt > 0)
			memmove(mbp->buf, &mbp->buf[size], mbp->byte_cnt);

		if(dbug_level & dbgVERBOSE)
			printf("[%s]\n", &(bp->data[1]));

		convrt2cr(bp->data, bp->cnt);
		send_ax25(mbp->axbbscb, bp);
	}

	if (mbp->byte_cnt == 0) {
		if(dbug_level & dbgVERBOSE)
			printf("axsend: %s re-enabling Nagle gate\n",
				mbp->call);
		mbp->nagle_gate_down = 1;
	}

	if (mbp->networkfull && mbp->byte_cnt < sizeof(mbp->buf)) {
		/*
		 * We have read notifications shut off from the network
		 * because the read buffer was full. The buffer is no longer
		 * full, so it's ok to re-enable read events.
		 */
		alEvent_setFdEvents(mbp->al_fd_handle, ALFD_READ);
		mbp->networkfull = 0;
	}
}

static void
shutdown_process(struct mboxsess *mbp, int issue_disc)
{
	if(dbug_level & dbgVERBOSE)
		printf("Shutdown process %d..\n", mbp->pid);
	if(mbp->spawned) {
		if(dbug_level & dbgVERBOSE)
			printf("Killing process %d..\n", mbp->pid);
		kill(mbp->pid,9);
	}

	if(mbp->fd != ERROR) {
		if(dbug_level & dbgVERBOSE)
			printf("closing fd %x..\n", mbp->fd);
		if (mbp->al_fd_handle != NULL)
			alEvent_deregister(mbp->al_fd_handle);
		close(mbp->fd);
	}
	if(mbp->socket != ERROR) {
		if(dbug_level & dbgVERBOSE)
			printf("closing socket %x..\n", mbp->socket);
		if (mbp->al_socket_handle != NULL)
			alEvent_deregister(mbp->al_socket_handle);
		close(mbp->socket);
	}

	if(issue_disc) {
		if(dbug_level & dbgVERBOSE)
			printf("Issue disconnect\n");
		if(mbp->axbbscb != NULL)
			disc_ax25(mbp->axbbscb);
	}
	if(mbp->nagle_timer_id != -1)
		alEvent_cancelTimer(mbp->nagle_timer_id);


	/* it should have died by now. Let's get it's completion
	 * status.
	 */

	if(mbp->spawned)
		waitpid(mbp->pid, NULL, WNOHANG);
		
#if 0
	if(mbp->orig)
		delete_call(mbp->orig);
#endif

	freembox(mbp);
	if(dbug_level & dbgVERBOSE)
		printf("Shutdown complete\n");
}

void
mbx_state(struct ax25_cb *axp, int old, int new)
{
	char call[10], port[10];
	int pid, j;
	struct mboxsess *mbp;
	alCallback cb;

	if(dbug_level & dbgVERBOSE)
		printf("mbx_state(%d, %d)\n", old, new);
	switch(new){
	case DISCONNECTED:
		if(dbug_level & dbgVERBOSE)
			printf("mbx_state(DISCONNECTED)\n");
		if((old == DISCONNECTED) || (old == DISCPENDING))
		   return;
		if(base == NULLMBS)
			break;
		if(dbug_level & dbgVERBOSE)
			printf("mbx_state(isDISCONNECTED)\n");
		mbp = base;
		while(mbp != NULLMBS){
			if(axp == mbp->axbbscb){
				if(dbug_level & dbgVERBOSE)
					printf("mbx_state:Killing BBS, disconnect recv\n");
				shutdown_process(mbp, FALSE);
				break; /* from while loop */
			}
			mbp = mbp->next;
		}
		break;   /*end of DISCONNECTED case*/
					
	case CONNECTED:
		switch(old) {
		case SETUP:
			mbp = base;
			while(mbp != NULLMBS && axp != mbp->axbbscb)
				mbp = mbp->next;
			if(mbp == NULLMBS) {
				if(dbug_level & dbgVERBOSE)
					printf("couldn't find the process\n");
				exit(1);
			}
			write(mbp->fd, "~C\n", 3);

			mbp->axbbscb=axp;
			mbx_notify_sendable(mbp, 500, 1); /*jump start the upcall*/
			return;

		case CONNECTED:
			/* we are already CONNECTED but received another SABM
			 * this means they probably didn't get our UA
			 */
			if(dbug_level & dbgVERBOSE)
				printf("Another SABM received, they must be deaf.\n");
			if(base == NULLMBS)
				break;

			mbp = base;
			while(mbp != NULLMBS){
				if(axp == mbp->axbbscb){
					if(dbug_level & dbgVERBOSE)
						printf("mbx_state:Killing BBS, disconnect recv\n");
					shutdown_process(mbp, FALSE);
					break; /* from while loop */
				}
				mbp = mbp->next;
			}
			break;

		default:
			return;

		case DISCONNECTED:
			break;
		}

		if(!calleq(axp,&bbscall)) { /*not for the mailbox*/
			axp->s_upcall = NULL;
			return;
		}
	
		if(dbug_level & dbgVERBOSE)
			printf("mbx_state(isCONNECTED)\n");
		mbp =newmbox();		/*after this, this is a mailbox connection*/
								/* so, make a new mailbox session*/
		axp->r_upcall = mbx_rx ;
		axp->t_upcall = mbx_tx ;

		mbp->axbbscb=axp;
		mbx_notify_sendable(mbp, 500, 1); /*jump start the upcall*/

		for(j=0;j<6;j++){			   /*now, get incoming call letters*/
			call[j]=mbp->axbbscb->addr.dest.call[j];
			call[j]=call[j] >> 1;
			call[j]=call[j] & (char)0x7f;
			if(call[j]==' ') call[j]='\0';
		}
		call[6]='\0';			/*terminate call letters*/
		strcpy(mbp->call,call);   /*Copy call to session*/

		if(mbp->pid == 0) {
			mbp->port = 0;
			mbp->socket = socket_listen(NULL, &(mbp->port));
				
			sprintf(port, "%d", mbp->port);

			AL_CALLBACK(&cb, mbp, mbx_accept);
			int res = alEvent_registerFd(mbp->socket, ALFD_READ,
				cb, &mbp->al_socket_handle);
			if (res != 0) {
				fprintf(stderr, "bbs accept register error\n");
				exit(1);
			}

			char addr[256];
			snprintf(addr, sizeof(addr), "ax25:%s-%d", call,
				mbp->axbbscb->addr.dest.ssid);

			if(dbug_level & dbgVERBOSE) {
				printf("Starting bbs process [1]:\n");
				printf("\t%s/b_bbs -v %s -e -s %s -a %s %s\n",
					Bin_Dir, Tncd_Name, port, addr, call);
			}

			/*now, fork and exec the bbs*/
			if((pid=fork()) == 0){			/*if we are the child*/
				char cmd[256];

				sprintf(cmd, "%s/b_bbs", Bin_Dir);
				execl(cmd, "b_bbs", "-v", Tncd_Name, "-s",
					port, "-e", "-a", addr, call, 0);
				exit(1);
			}

			mbp->pid=pid;				/* save pid of new baby*/
			mbp->spawned = TRUE;

			AL_CALLBACK(&cb, mbp, mbx_handle_child_exit);
			res = alEvent_registerProc(mbp->pid, ALPROC_EXIT, cb,
				&mbp->al_proc_handle);
			if (res != 0) {
				fprintf(stderr, "bbs wait register error\n");
				exit(1);
			}
		} else {
			char *buf = "CONNECTED\n";
			write(mbp->fd, buf, strlen(buf));
		}
		break;
	}
}

/* Incoming mailbox session via ax.25 */

/* * This is the ax25 receive upcall function
 *it gathers incoming data and stuff it down the IPC queue to the proper BBS
 */

/*ARGSUSED*/
void
mbx_incom(struct ax25_cb *axp, int cnt)
{
	char call[10], port[10];
	int pid, j;
	struct mboxsess *mbp;
	struct mbuf *bp;
	
	if(dbug_level & dbgVERBOSE)
		printf("mbx_incom()\n");

	mbp = newmbox();	/*after this, this is a mailbox connection*/
						/* so, make a new mailbox session*/
	axp->r_upcall = mbx_rx ;
	axp->t_upcall = mbx_tx ;
	axp->s_upcall = mbx_state;

	mbp->axbbscb=axp;
	mbx_notify_sendable(mbp, 500, 1); /*jump start the upcall*/

	bp = recv_ax25(axp) ;	/* get the initial input */
	free_p(bp) ;			/* and throw it away to avoid confusion */


	for(j=0;j<6;j++){			   /*now, get incoming call letters*/
		call[j] = mbp->axbbscb->addr.dest.call[j];
		call[j] = call[j] >> 1;
		call[j] = call[j] & (char)0x7f;
		if(call[j]==' ')
			call[j]='\0';
	}
	call[6]='\0';					/*terminate call letters*/
	strcpy(mbp->call, call);   		/*Copy call to session*/

	mbp->fd = ERROR;
	mbp->port = 0;
	mbp->socket = socket_listen(NULL, &(mbp->port));
	sprintf(port, "%d", mbp->port);

	char addr[256];
	snprintf(addr, sizeof(addr), "ax25:%s-%d", call,
		mbp->axbbscb->addr.dest.ssid);

	/*now, fork and exec the bbs*/
 
	if(dbug_level & dbgVERBOSE) {
		printf("Starting bbs process [2]:\n");
		printf("\t%s/b_bbs -v %s -s %s -a %s %s\n",
			Bin_Dir, Tncd_Name, port, addr, call);
	}

	if((pid=fork()) == 0){			/*if we are the child*/
		char cmd[256];
		sprintf(cmd, "%s/b_bbs", Bin_Dir);
		execl(cmd, "b_bbs", "-v", Tncd_Name, "-e", "-s", port, call, 0);
		exit(1);
	}
	mbp->pid=pid;				/* save pid of new baby*/
	mbp->spawned = TRUE;
}	

/*ARGSUSED*/
static void
mbx_rx(struct ax25_cb *axp, int cnt)
{
	struct mbuf *bp;
	struct mboxsess * mbp;

	
	if(base == NULLMBS)
		return;

	if(dbug_level & dbgVERBOSE)
		printf("mbx_rx()\n");
	mbp = base;
	while(mbp != NULLMBS){
		if(mbp->axbbscb == axp){  /* match requested block? */
			if((bp = recv_ax25(axp)) == NULLBUF)  /*nothing there*/
				continue;
			while(bp != NULLBUF){
				convrt2nl(bp->data, bp->cnt);
				if(dbug_level & dbgVERBOSE)
					write(2, bp->data, bp->cnt);
				write(mbp->fd, bp->data, bp->cnt);
				bp = free_mbuf(bp);   /*free the mbuf and get the next */
			}
		}
	mbp = mbp->next;
	}
}

static void
mbx_tx(struct ax25_cb *axp, int cnt)
{
	struct mboxsess *mbp;
	if(base == NULLMBS)
		return;					 /*no sessions*/

	if(dbug_level & dbgVERBOSE)
		printf("mbx_tx()\n");
	mbp = base;
	while(mbp != NULLMBS){
		if(mbp->axbbscb == axp)
			mbx_notify_sendable(mbp, cnt, 1);
		mbp = mbp->next;
	}
}

static int
calleq(struct ax25_cb *axp, struct ax25_addr *addr)
{
	if(memcmp((char*)axp->addr.source.call, (char*)addr->call, ALEN) != 0)
		return 0;
	if((axp->addr.source.ssid & SSID) != (addr->ssid & SSID))
		return 0;
	return 1;
}

static void
convrt2nl(char *data, int cnt)
{
	int i;
	for(i=0; i<cnt; i++) {
		if(*data == '\r')
			*data = '\n';
		data++;
	}
}

static void
convrt2cr(char *data, int cnt)
{
	int i;
	for(i=0; i<cnt; i++) {
		if(*data == '\n')
			*data = '\r';
		data++;
	}
}

/*
 * bbs process exited before it established a connection with
 * us.
 */
static void
mbx_handle_child_exit(void *obj, void *arg0, int arg1)
{
	struct mboxsess *mbp = obj;
	pid_t res;

	assert(mbp->al_proc_handle == (alEventHandle)arg0);

	res = waitpid(mbp->pid, NULL, WNOHANG);
	assert(res == mbp->pid);

	fprintf(stderr, "bbs exited before making control connection.\n");

	mbp->pid = 0;
	mbp->spawned = FALSE;

	alEvent_deregister(mbp->al_proc_handle);
	mbp->al_proc_handle = NULL;

	shutdown_process(mbp, TRUE);
}

/*
 * A previously congested LAPB connection send queue has cleared up and more
 * bytes can be sent.
 */
static void
mbx_notify_sendable(struct mboxsess *mbp, size_t amount, int queue)
{
	mbp->sendable_count = amount;

	if (queue) {
		alCallback cb;
		AL_CALLBACK(&cb, mbp, mbx_sendable_changed);
		alEvent_queueCallback(cb, ALCB_UNIQUE, NULL, 0);
	}
}

static void
mbx_sendable_changed(void *mbpp, void *arg0, int arg1)
{
	struct mboxsess *mbp = mbpp;
	mbx_process_tx_queue(mbp);
}

static void
mbx_nagle_timer(void *obj, void *arg0, int arg1)
{
	struct mboxsess *mbp = obj;

	assert(mbp->nagle_timer_id != -1);
	assert(mbp->nagle_gate_down == 1);

	mbp->nagle_timer_id = -1;
	mbp->nagle_gate_down = 0;

	if(dbug_level & dbgVERBOSE)
		printf("axsend: %s lifting Nagle gate due to timeout\n",
			mbp->call);

	mbx_process_tx_queue(mbp);
}
