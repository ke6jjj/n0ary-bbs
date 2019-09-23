/* Send and receive IP datagrams on serial lines. Compatible with SLIP
 * under Berkeley Unix.
 */
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"
#include "mbuf.h"
#include "slip.h"

/* SLIP definitions */
#define	SLIP_ALLOC	1024	/* Receiver allocation increment */

#define	FR_END		0300	/* Frame End */
#define	FR_ESC		0333	/* Frame Escape */
#define	T_FR_END	0334	/* Transposed frame end */
#define	T_FR_ESC	0335	/* Transposed frame escape */

/* Slip protocol control structure */
struct slip {
	char escaped, flags;	/* Receiver State control flag */
	struct mbuf *rbp;	/* Head of mbuf chain being filled */
	struct mbuf *rbp1;	/* Pointer to mbuf currently being written */
	unsigned char *rcp;	/* Write pointer */
	int rcnt;		/* Length of mbuf chain */
	int errors;		/* Receiver input errors */

	/* Receiver of decoded SLIP frames */
	mbuf_recv_fn recv;
	void *recv_arg;

	/* Receiver of encoded SLIP frames */
	mbuf_recv_fn send;
	void *send_arg;
};

static struct mbuf *slip_encode(struct mbuf *bp, int flags);
static struct mbuf *slip_decode(slip *slip, u_char c);

slip *
slip_init(int flags)
{
	slip *sp = malloc_struct(slip);

	if (sp == NULL)
		return NULL;

	sp->rbp = NULL;
	sp->rbp1 = NULL;
	sp->rcnt = 0;
	sp->errors = 0;
	sp->flags = flags;

	return sp;
}

int
slip_set_send(slip *sp, mbuf_recv_fn send, void *arg)
{
	sp->send = send;
	sp->send_arg = arg;
	return 0;
}

int
slip_set_recv(slip *sp, mbuf_recv_fn recv, void *arg)
{
	sp->recv = recv;
	sp->recv_arg = arg;
	return 0;
}

/* Process SLIP line I/O */
int
slip_input(void *spp, const char *buf, size_t sz)
{
	slip *sp = (struct slip *) spp;
	size_t i;
	struct mbuf *bp;

	/* Process any pending input */
	for (i = 0; i < sz; i++) {
		if((bp = slip_decode(sp, buf[i])) != NULLBUF)
			sp->recv(sp->recv_arg, bp);
	}

	return 0;
}

/* Encode a raw packet in slip framing, put on link output queue, and kick
 * transmitter
 */
int
slip_output(void *spp, struct mbuf *data)
{
	slip *sp = (struct slip *) spp;
	struct mbuf *bp;

	if((bp = slip_encode(data, sp->flags)) == NULLBUF)
		return -1;	

	return sp->send(sp->send_arg, bp);
}

void
slip_deinit(slip *sp)
{
	if (sp->rbp != NULL)
		free_p(sp->rbp);
	free(sp);
}
	

/* Process incoming bytes in SLIP format
 * When a buffer is complete, return it; otherwise NULLBUF
 */
static struct mbuf *
slip_decode(slip *sp, u_char c)
{
	struct mbuf *bp;

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

/* Encode a packet in SLIP format */
static struct mbuf *
slip_encode(struct mbuf *bp, int flags)
{
	struct mbuf *lbp;	/* Mbuf containing line-ready packet */
	unsigned char *cp;
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
