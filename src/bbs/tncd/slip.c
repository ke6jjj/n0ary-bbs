/* Send and receive IP datagrams on serial lines. Compatible with SLIP
 * under Berkeley Unix.
 */
#include <stdio.h>
#include <sys/types.h>
#include "mbuf.h"
#include "kiss.h"
#include "bsd.h"
#include "slip.h"
#include "tnc.h"

static struct mbuf
	*slip_encode(struct mbuf *bp),
	*slip_decode(int dev, u_char c);

static void
	asy_start(int dev);

static int
	slipq(int dev, struct mbuf *data);

/* Slip level control structure */
struct slip slip[MAX_TNC];


/* Start output, if possible, on asynch device dev */
static void
asy_start(int dev)
{
	register struct slip *sp;

	sp = &slip[dev];
	if(sp->tbp != NULLBUF){
		/* transmission just completed */
		free_p(sp->tbp);
		sp->tbp = NULLBUF;
	}
	if(sp->sndq == NULLBUF)
		return;	/* No work */

	sp->tbp = dequeue(&sp->sndq);
	sp->sndcnt--;
	asy_output(dev, sp->tbp->data, sp->tbp->cnt);
}


/* Process incoming bytes in SLIP format
 * When a buffer is complete, return it; otherwise NULLBUF
 */
static struct mbuf *
slip_decode(int dev, u_char c)
{
	struct mbuf *bp;
	struct slip *sp;

	sp = &slip[dev];

	switch(c) {
	case FR_END:
		bp = sp->rbp;
		sp->rbp = NULLBUF;
		sp->rcnt = 0;
		return bp;	/* Will be NULLBUF if empty frame */

	case FR_ESC:
		sp->escaped = 1;
		return NULLBUF;
	}

	if(sp->escaped) {
		sp->escaped = 0;
		switch(c) {
		case T_FR_ESC:
			c = FR_ESC;
			break;

		case T_FR_END:
			c = FR_END;
			break;

		default:
			sp->errors++;
			break;
		}
	}

	if(sp->rbp == NULLBUF) {
					/* Allocate first mbuf for new packet */
		if((sp->rbp1 = sp->rbp = alloc_mbuf(SLIP_ALLOC)) == NULLBUF)
			return NULLBUF;
		sp->rcp = sp->rbp->data;
	} else
		if(sp->rbp1->cnt == SLIP_ALLOC) {
					/* Current mbuf is full; link in another */
		if((sp->rbp1->next = alloc_mbuf(SLIP_ALLOC)) == NULLBUF){
					/* No memory, drop whole thing */
			free_p(sp->rbp);
			sp->rbp = NULLBUF;
			sp->rcnt = 0;
			return NULLBUF;
		}
		sp->rbp1 = sp->rbp1->next;
		sp->rcp = sp->rbp1->data;
	}
			/* Store the character, increment fragment and total byte counts */
	*sp->rcp++ = c;
	sp->rbp1->cnt++;
	sp->rcnt++;
	return NULLBUF;
}

/* Process SLIP line I/O */
void
doslip(int dev)
{
	char c;
	struct mbuf *bp;

			/* Process any pending input */
	while(asy_recv(dev, &c, 1) != 0)
		if((bp = slip_decode(dev, c)) != NULLBUF)
			kiss_recv(dev, bp);

			/* Kick the transmitter if it's idle */
	asy_start(dev);
}


/* Send a raw slip frame -- also trivial */
int
slip_raw(int dev, struct mbuf *data)
{
	/* Queue a frame on the slip output queue and start transmitter */
	return slipq(dev,data);
}

/* Encode a raw packet in slip framing, put on link output queue, and kick
 * transmitter
 */
static int
slipq(int dev, struct mbuf *data)
{
	struct slip *sp;
	struct mbuf *bp;

	if((bp = slip_encode(data)) == NULLBUF)
		return -1;	

	sp = &slip[dev];
	enqueue(&sp->sndq,bp);
	sp->sndcnt++;
	if(sp->tbp == NULLBUF)
		asy_start(dev);
	return 0;
}

/* Encode a packet in SLIP format */
static struct mbuf *
slip_encode(struct mbuf *bp)
{
	struct mbuf *lbp;	/* Mbuf containing line-ready packet */
	register char *cp;
	char c;

	/* Allocate output mbuf that's twice as long as the packet.
	 * This is a worst-case guess (consider a packet full of FR_ENDs!)
	 */
	lbp = alloc_mbuf(2*len_mbuf(bp) + 2);
	if(lbp == NULLBUF){
		/* No space; drop */
		free_p(bp);
		return NULLBUF;
	}
	cp = lbp->data;

	/* Flush out any line garbage */
	*cp++ = FR_END;

	/* Copy input to output, escaping special characters */
	while(pullup(&bp,&c,1) == 1){
		switch((u_char)(c)){
		case FR_ESC:
			*cp++ = FR_ESC;
			*cp++ = T_FR_ESC;
			break;
		case FR_END:
			*cp++ = FR_ESC;
			*cp++ = T_FR_END;
			break;
		default:
			*cp++ = c;
		}
	}
	*cp++ = FR_END;
	lbp->cnt = cp - lbp->data;
	return lbp;
}
