/* User subroutines for AX.25 */
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "global.h"
#include "mbuf.h"
#include "timer.h"
#include "ax25.h"

/* Open an AX.25 connection */
struct ax25_cb *
open_ax25(
	struct ax25 *addr,		/* Addresses */
	int window,			/* Window size in bytes */
	void (*r_upcall)(),		/* Receiver upcall handler */
	void (*t_upcall)(),		/* Transmitter upcall handler */
	void (*s_upcall)(),		/* State-change upcall handler */
	kiss *dev,
	void *user)			/* User linkage area */
{
	struct ax25_cb *axp;

	if((axp = cr_ax25(dev, &addr->source, &addr->dest)) == NULLAX25)
		return NULLAX25;
	memcpy(&(axp->addr), addr, sizeof(axp->addr));
	bcopy((unsigned char *)addr, (unsigned char*)&(axp->addr), sizeof(struct ax25));
	if(addr->ndigis != 0){
		axp->t1.start *= (addr->ndigis + 1);
		axp->t2.start *= (addr->ndigis + 1);
		axp->t3.start *= (addr->ndigis + 1);
	}
	axp->window = window;
	axp->r_upcall = r_upcall;
	axp->t_upcall = t_upcall;
	axp->s_upcall = s_upcall;
	axp->user = user;

	switch(axp->state){
	case DISCONNECTED:
		/* Don't send anything if the connection already exists */
		est_link(axp);
		lapbstate(axp,SETUP);
		break;
	case SETUP:
		free_q(&axp->txq);
		break;
	case DISCPENDING:	/* Ignore */
	case FRAMEREJECT:
		break;
	case RECOVERY:
	case CONNECTED:
		free_q(&axp->txq);
		est_link(axp);
		lapbstate(axp,SETUP);
		break;
	}
	return axp;
}

/* Send data on an AX.25 connection. Caller must provide PID */
int
send_ax25(struct ax25_cb *axp, struct mbuf *bp)
{
	if(axp == NULLAX25 || bp == NULLBUF)
		return -1;
	enqueue(&axp->txq,bp);

	/* Kick the send timer */
	if (!run_timer(&axp->t2)) {
		start_timer(&axp->t2);
	}
	return 0;
}

/* Receive incoming data on an AX.25 connection */
struct mbuf *
recv_ax25(struct ax25_cb *axp)
{
	struct mbuf *bp;

	if(axp->rxq == NULLBUF)
		return NULLBUF;

	bp = axp->rxq;
	axp->rxq = NULLBUF;

	/* If this has un-busied us, send a RR to reopen the window */
	if(len_mbuf(bp) >= axp->window) {
		if (! run_timer(&axp->t2)) {
			axp->response = RR;
			start_timer(&axp->t2);
		}
	}
	return bp;
}

/* Close an AX.25 connection */
void
disc_ax25(struct ax25_cb *axp)
{
	switch(axp->state){
	case DISCONNECTED:
		break;		/* Ignored */
	case DISCPENDING:
		lapbstate(axp,DISCONNECTED);
		del_ax25(axp);
		break;
	case SETUP:
	case CONNECTED:
	case RECOVERY:
	case FRAMEREJECT:
		free_q(&axp->txq);
		axp->retries = 0;
		sendctl(axp,COMMAND,DISC|PF);
		stop_timer(&axp->t3);
		start_timer(&axp->t1);
		lapbstate(axp,DISCPENDING);
		break;
	}
}
