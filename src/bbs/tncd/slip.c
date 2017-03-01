/* Send and receive IP datagrams on serial lines. Compatible with SLIP
 * under Berkeley Unix.
 */
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>

#include "alib.h"
#include "mbuf.h"
#include "kiss.h"
#include "bsd.h"
#include "slip.h"
#include "tnc.h"

/* SLIP definitions */
#define	SLIP_ALLOC	1024	/* Receiver allocation increment */

#define	FR_END		0300	/* Frame End */
#define	FR_ESC		0333	/* Frame Escape */
#define	T_FR_END	0334	/* Transposed frame end */
#define	T_FR_ESC	0335	/* Transposed frame escape */

/* Slip protocol control structure */
struct slip {
	struct mbuf *sndq;	/* Encapsulated packets awaiting transmission */
	int sndcnt;		/* Number of datagrams on queue */
	char escaped, flags;	/* Receiver State control flag */
	struct mbuf *rbp;	/* Head of mbuf chain being filled */
	struct mbuf *rbp1;	/* Pointer to mbuf currently being written */
	char *rcp;		/* Write pointer */
	int rcnt;		/* Length of mbuf chain */
	struct mbuf *tbp;	/* Transmit mbuf being sent */
	int errors;		/* Receiver input errors */
	void (*recv)();		/* Function to call with an incoming buffer */
	alCallback selfChkQCb;  /* Callback handle to self for dequeue task */
};

static struct mbuf *slip_encode(struct mbuf *bp, int flags);
static struct mbuf *slip_decode(int dev, u_char c);
static void slip_check_queue(void *obj, void *arg0, int arg1);
static void asy_start(int dev);
static void doslip(int dev, void *unused);
static int slipq(int dev, struct mbuf *data);

/* Slip level control structure */
struct slip slip[MAX_TNC];

int Tncd_SLIP_Flags;

int
slip_init(int dev)
{
	struct slip *sp;

	if (dev < 0 || dev >= MAX_TNC)
		return -1;

	sp = &slip[dev];

	sp->tbp = NULLBUF;
	sp->sndq = NULLBUF;
	sp->sndcnt = 0;
	sp->rcnt = 0;
	sp->errors = 0;
	AL_CALLBACK(&sp->selfChkQCb, sp, slip_check_queue);

	return 0;
}

int
slip_start(int dev)
{
	return asy_set_read_cb(dev, doslip, NULL);
}

int
slip_stop(int dev)
{
	return asy_set_read_cb(dev, NULL, NULL);
}
	

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

	/* Keep calling back until queue is empty */
	alEvent_queueCallback(sp->selfChkQCb, ALCB_UNIQUE, NULL, dev);
}

static void
slip_check_queue(void *obj, void *arg0, int arg1)
{
	struct slip *sp = obj;
	int dev = arg1;

	assert(&slip[dev] == sp);

	asy_start(dev);
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
static void
doslip(int dev, void *unused)
{
	char buf[128];
	int count, i;
	struct mbuf *bp;

	(void)unused;

	/* Process any pending input */
	while((count = asy_recv(dev, buf, sizeof(buf))) > 0) {
		for (i = 0; i < count; i++) {
			if((bp = slip_decode(dev, buf[i])) != NULLBUF)
				kiss_recv(dev, bp);
		}
	}
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

	if((bp = slip_encode(data, Tncd_SLIP_Flags)) == NULLBUF)
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
slip_encode(struct mbuf *bp, int flags)
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
		case 'C':
			/* Handle ASCII C for weird KISS problem */
			if (flags & SLIP_FLAGS_ESCAPE_ASCII_C) {
				*cp++ = FR_ESC;
				*cp++ = 'C';
				break;
			}
			/*FALLTHROUGH*/
		default:
			*cp++ = c;
		}
	}
	*cp++ = FR_END;
	lbp->cnt = cp - lbp->data;
	return lbp;
}
