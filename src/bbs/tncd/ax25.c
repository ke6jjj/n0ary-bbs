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

extern char
	*Bbs_My_Call,
	*Bbs_Fwd_Call;

struct ax25_addr mycall, bbscall, fwdcall;
int digipeat = 1;	/* Controls digipeating */

void
build_bbscall(void)
{
	char *s = Bbs_My_Call;
	int i = 0;

	while(isalnum(*s)) {
		bbscall.call[i] = *s++;
		ToUpper(bbscall.call[i]);
		bbscall.call[i++] <<= 1;
	}

	for(;i<ALEN; i++)
		bbscall.call[i] = ' ' << 1;

	if(*s == '-')
		s++;

	if(*s == 0)
		bbscall.ssid = ('0' << 1) | E;
	else
		bbscall.ssid = (*s << 1) | E;

#if 0
	s = Bbs_Fwd_Call;
	i = 0;

	while(isalnum(*s)) {
		fwdcall.call[i] = *s++;
		ToUpper(fwdcall.call[i]);
		fwdcall.call[i++] <<= 1;
	}

	for(;i<ALEN; i++)
		fwdcall.call[i] = ' ' << 1;

	if(*s == '-')
		s++;

	if(*s == 0)
		fwdcall.ssid = ('0' << 1) | E;
	else
		fwdcall.ssid = (*s << 1) | E;
#endif
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

	if(axp == NULLAX25 || tnc[axp->dev].inuse == FALSE)
		return ERROR;

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
 * us or to QST-0, kick it upstairs depending on the protocol ID.
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
			 * originated the call from our end.
			 */

	if(addreq(&hdr.dest,&bbscall) == FALSE)
		if(scan_calls(&hdr.dest, &hdr.source) == FALSE) {
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
		if(dbug_level & dbgVERBOSE)
			printf("ax25.c:ax_recv why am I here 'if(control == UI)'\n");
		free_p(bp);
		return;
	}

		/* Everything from here down is LAPB stuff, so drop anything
		 * not directed to us:
		 */

		/* Find the source address in hash table */

	if((axp = find_ax25(&hdr.dest, &hdr.source)) == NULLAX25){
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
		memcpy((char*)&axp->addr.dest, (char*)&hdr.source, sizeof(struct ax25_addr));
		memcpy((char*)&axp->addr.source, (char*)&hdr.dest, sizeof(struct ax25_addr));

		if(hdr.ndigis > 0){
			int i,j;
					/* Construct reverse digipeater path */
			for(i=hdr.ndigis-1,j=0; i >= 0; i--,j++) {
				memcpy((char*)&axp->addr.digis[j], (char*)&hdr.digis[i], 
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
