/* Low level AX.25 frame processing - address header */

#include <stdio.h>
#include <sys/types.h>
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "timer.h"
#include "mbuf.h"
#include "kiss.h"
#include "ax25.h"
#include "tnc.h"
/*
 * Including bsd.h just so we can quickly cobble together the transmit
 * unlock feature in the variable Tncd_TX_Enable.
 */
#include "bsd.h"

extern char
	*Bbs_My_Call,
	*Bbs_Unlock_Call;

struct ax25_addr bbscall, unlockcall;
int digipeat = 1;	/* Controls digipeating */
int unlockable = 0;

void
build_bbscall(void)
{
	setcall(&bbscall, Bbs_My_Call);
	if (setcall(&unlockcall, Bbs_Unlock_Call) == 0)
		unlockable = 1;
}

/* Initialize AX.25 entry in arp device table */
/* General purpose AX.25 frame output */
int
sendframe(
	struct ax25_cb *axp,
	char cmdrsp,
	char ctl,
	struct mbuf *data)
{
	struct mbuf *hbp,*cbp;

	if(axp == NULLAX25 || tnc[axp->dev].inuse == FALSE) {
		free_p(data);
		return ERROR;
	}

				/* Add control field */
	if((cbp = pushdown(data, 1)) == NULLBUF){
		free_p(data);
		return ERROR;
	}
	cbp->data[0] = ctl;

	axp->addr.cmdrsp = cmdrsp;
				/* Create address header */
	if((hbp = htonax25(&axp->addr, cbp)) == NULLBUF){
		free_p(cbp);
		return ERROR;
	}
				/* The packet is all ready, now send it */
	return kiss_raw(axp->dev, hbp);
}


/* Process incoming AX.25 packets.
 * After optional tracing, the address field is examined. If it is
 * directed to us as a digipeater, repeat it.  If it is addressed to
 * us and is LAPB, kick it upstairs.
 */
void
ax_recv(int dev, struct mbuf *bp)
{
	struct ax25_addr *ap;
	struct mbuf *hbp;
	u_char control;
	struct ax25 hdr;
	struct ax25_cb *axp;

	/* Pull header off packet and convert to host structure */
	/* returns -1 if too many digis */
	if(ntohax25(&hdr, &bp) < 0){
		free_p(bp);
		return;
	}

	/* Emergency TX inhibit unlock feature */
	if (Tncd_TX_Enabled == 0 && unlockable == 1 &&
	    addreq(&hdr.dest, &unlockcall))
		Tncd_TX_Enabled = 1;

	/* Scan, looking for our call in the repeater fields, if any.
	 * Repeat appropriate packets.
	 */
	for(ap=&hdr.digis[0]; ap<&hdr.digis[hdr.ndigis]; ap++){
		if(ap->ssid & REPEATED)
			continue; /* Already repeated */

		/* Check if packet is directed to us as a digipeater */
		if(digipeat && addreq(ap, &bbscall)){
			/* Yes, kick it back out */
			ap->ssid |= REPEATED;
			if((hbp = htonax25(&hdr, bp)) != NULLBUF){
				kiss_raw(dev, hbp);
				bp = NULLBUF;
			}
		}
		free_p(bp);	/* Dispose if not forwarded */
		return;
	}

	/* Packet has passed all repeaters, now look at destination */

	/* all hits are allowed on "bbscall" address, this is where we
	 * accept connects. All other addresses are only passed if we
	 * originated the call from our end (which is determined if
	 * we find a match for the address pair).
	 */
	axp = find_ax25(&hdr.dest, &hdr.source);
	if(axp == NULLAX25 && addreq(&hdr.dest,&bbscall) == FALSE){
		/* Not for us */
		free_p(bp);
		return;
	}

	if(bp == NULLBUF){
		/* Nothing left */
		return;
	}

	/* Sneak a peek at the control field. This kludge is necessary because
	 * AX.25 lacks a proper protocol ID field between the address and LAPB
	 * sublayers; a control value of UI indicates that LAPB is to be
	 * bypassed.
	 */
	control = *bp->data & ~PF;

	if(control == UI){
		/* This would happen if we were to receive, say, a TCP/IP
		 * packet. But we wouldn't.
		 */
		free_p(bp);
		return;
	}

	/* Everything from here down is LAPB stuff. */

	/* Find the source address in hash table */

	if(axp == NULLAX25){
		/* Create a new ax25 entry for this guy,
		 * insert into hash table keyed on his address,
		 * and initialize table entries
		 */
		if((axp = cr_ax25(dev, &hdr.dest, &hdr.source)) == NULLAX25){
			free_p(bp);
			return;
		}

		axp->dev = dev;

		/* Swap source and destination, reverse digi string */
		memcpy(&axp->addr.dest, &hdr.source, sizeof(struct ax25_addr));
		memcpy(&axp->addr.source, &hdr.dest, sizeof(struct ax25_addr));

		if(hdr.ndigis > 0){
			int i,j;
			/* Construct reverse digipeater path */
			for(i=hdr.ndigis-1,j=0; i >= 0; i--,j++) {
				memcpy(&axp->addr.digis[j], &hdr.digis[i], 
					sizeof(struct ax25_addr));
				axp->addr.digis[j].ssid &= ~(E|REPEATED);
			}

			/* Scale timers to account for extra delay */
			axp->t1.start *= hdr.ndigis+1;
			axp->t2.start *= hdr.ndigis+1;
			axp->t3.start *= hdr.ndigis+1;
		}
		axp->addr.ndigis = hdr.ndigis;
	}

	if(hdr.cmdrsp == UNKNOWN)
		axp->proto = V1;	/* Old protocol in use */
	else
		axp->proto = V2;

	lapb_input(axp, hdr.cmdrsp, bp);
}
