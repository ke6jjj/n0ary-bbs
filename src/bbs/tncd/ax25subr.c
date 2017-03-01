#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "timer.h"
#include "mbuf.h"
#include "ax25.h"

#include "ax_mbx.h"

struct ax25_cb *ax25_cb[NHASH];

int axwindow = 2048;		/* 2K incoming text before RNR'ing */

		/* Address hash function. Exclusive-ORs each byte, ignoring
		 * such insignificant, annoying things as E and H bits
		 */
static int
ax25hash(struct ax25_addr *s)
{
	register char x;
	register int i;
	register unsigned char *cp;

	x = 0;
	cp = s->call;
	for(i=ALEN; i!=0; i--)
		x ^= *cp++ & 0xfe;
	x ^= s->ssid & SSID;
	return (u_char)(x) % NHASH;
}

		/* Look up entry in hash table */

struct ax25_cb *
find_ax25(struct ax25_addr *my_addr, struct ax25_addr *their_addr)
{
	int hashval;
	struct ax25_cb *axp;

				/* Find appropriate hash chain */
	hashval = ax25hash(their_addr);

						/* Search hash chain */
	for(axp = ax25_cb[hashval]; axp != NULLAX25; axp = axp->next){
		if(addreq(&axp->addr.dest, their_addr))
			if(addreq(&axp->addr.source, my_addr))
				return axp;
	}
	return NULLAX25;
}

int
scan_calls(struct ax25_addr *us, struct ax25_addr *them)
{
	int hashval;
	struct ax25_cb *axp;

				/* Find appropriate hash chain */
	hashval = ax25hash(them);

						/* Search hash chain */
	for(axp = ax25_cb[hashval]; axp != NULLAX25; axp = axp->next){
		if(addreq(&axp->addr.dest, them))
			if(addreq(&axp->addr.source, us))
				return TRUE;
	}
	return FALSE;
}

		/* Remove address entry from hash table */
void
del_ax25(struct ax25_cb *axp)
{
	int hashval;

	if(axp == NULLAX25)
		return;
	/* Remove from hash header list if first on chain */
	hashval = ax25hash(&axp->addr.dest);

	/* Remove from chain list */
	if(ax25_cb[hashval] == axp)
		ax25_cb[hashval] = axp->next;
	if(axp->prev != NULLAX25)
		axp->prev->next = axp->next;
	if(axp->next != NULLAX25)
		axp->next->prev = axp->prev;

	/* Timers should already be stopped, but just in case... */
	stop_timer(&axp->t1);
	stop_timer(&axp->t2);
	stop_timer(&axp->t3);

	/* Free allocated resources */
	free_q(&axp->txq);
	free_q(&axp->rxasm);
	free_q(&axp->rxq);
	free((char *)axp);
}

/* Create an ax25 control block. Allocate a new structure, if necessary,
 * and fill it with all the defaults. The caller
 * is still responsible for filling in the reply address
 */
struct ax25_cb *
cr_ax25(int dev, struct ax25_addr *my_addr, struct ax25_addr *their_addr)
{
	register struct ax25_cb *axp;
	int hashval;

	if(their_addr == NULLAXADDR)
		return NULLAX25;

	if((axp = find_ax25(my_addr, their_addr)) == NULLAX25){
		/* Not already in table; create an entry
		 * and insert it at the head of the chain
 		 */

		 /* Find appropriate hash chain */
		hashval = ax25hash(their_addr);
		axp = (struct ax25_cb *)calloc(1,sizeof(struct ax25_cb));
		if(axp == NULLAX25)
			return NULLAX25;

		init_timer(&axp->t1);
		init_timer(&axp->t2);
		init_timer(&axp->t3);

		/* Insert at beginning of chain */
		axp->prev = NULLAX25;
		axp->next = ax25_cb[hashval];
		if(axp->next != NULLAX25)
			axp->next->prev = axp;
		ax25_cb[hashval] = axp;
	}
	axp->maxframe = Tncd_Maxframe;
	axp->window = axwindow;
	axp->paclen = Tncd_Paclen;
	axp->proto = V2;	/* Default, can be changed by other end */
	axp->pthresh = Tncd_Pthresh;
	axp->n2 = Tncd_N2;
	axp->t1.start = Tncd_T1init;
	axp->t1.func = recover;
	axp->t1.arg = (char *)axp;

	axp->t2.start = Tncd_T2init;
	axp->t2.func = send_ack;
	axp->t2.arg = (char *)axp;

	axp->t3.start = Tncd_T3init;
	axp->t3.func = pollthem;
	axp->t3.arg = (char *)axp;

	axp->s_upcall = mbx_state;
	axp->r_upcall = mbx_incom;

	axp->dev = dev;
	return axp;
}

/*
 * setcall - convert callsign plus substation ID of the form
 * "KA9Q-0" to AX.25 (shifted) address format
 *   Address extension bit is left clear
 *   Return -1 on error, 0 if OK
 */
int
setcall(struct ax25_addr *out, char *call)
{
	int csize;
	unsigned ssid;
	register int i;
	unsigned char *cp,*dp;
	char c;

	if(out == (struct ax25_addr *)NULL || call == NULL || *call == '\0'){
		return -1;
	}
	/* Find dash, if any, separating callsign from ssid
	 * Then compute length of callsign field and make sure
	 * it isn't excessive
	 */

	dp = (unsigned char*)index(call, '-');
	if(dp == NULL)
		csize = strlen(call);
	else
		csize = (int)dp - (int)call;
	if(csize > ALEN)
		return -1;
	/* Now find and convert ssid, if any */
	if(dp != NULL){
		dp++;	/* skip dash */
		ssid = atoi((char*)dp);
		if(ssid > 15)
			return -1;
	} else
		ssid = 0;
	/* Copy upper-case callsign, left shifted one bit */
	cp = out->call;
	for(i=0;i<csize;i++){
		c = *call++;
		if(islower(c))
			c = toupper(c);
		*cp++ = c << 1;
	}
	/* Pad with shifted spaces if necessary */
	for(;i<ALEN;i++)
		*cp++ = ' ' << 1;
	
	/* Insert substation ID field and set reserved bits */
	out->ssid = 0x60 | (ssid << 1);
	return 0;
}

int
addreq(struct ax25_addr *a, struct ax25_addr *b)
{
	if(memcmp((char*)a->call, (char*)b->call, ALEN) != 0)
		return FALSE;
	if((a->ssid & SSID) != (b->ssid & SSID))
		return FALSE;
	return TRUE;
}

/* Convert encoded AX.25 address to printable string */
void
pax25(char *e, struct ax25_addr *addr)
{
	register int i;
	unsigned char c,*cp;

	cp = addr->call;
	for(i=ALEN;i != 0;i--){
		c = (*cp++ >> 1) & 0x7f;
		if(c == ' ')
			break;
		*e++ = c;
	}
	if ((addr->ssid & SSID) != 0)
		sprintf(e,"-%d",(addr->ssid >> 1) & 0xf);	/* ssid */
	else
		*e = 0;
}

char *
getaxaddr(struct ax25_addr *ap, char *cp)
{
	memcpy((char*)ap->call,cp,ALEN);
	cp += ALEN;
	ap->ssid = *cp++;
	return cp;
}

static char *
putaxaddr(char *cp, struct ax25_addr *ap)
{
	memcpy(cp,(char*)ap->call,ALEN);
	cp += ALEN;
	*cp++ = ap->ssid;
	return cp;
}

/* Convert a host-format AX.25 header into a mbuf ready for transmission */
struct mbuf *
htonax25(struct ax25 *hdr, struct mbuf *data)
{
	struct mbuf *bp;
	register char *cp;
	register int i;

	if(hdr == (struct ax25 *)NULL || hdr->ndigis > MAXDIGIS)
		return NULLBUF;

	/* Allocate space for return buffer */
	i = AXALEN * (2 + hdr->ndigis);
	if((bp = pushdown(data, i)) == NULLBUF)
		return NULLBUF;

	/* Now convert */
	cp = bp->data;

	hdr->dest.ssid &= ~E;	/* Dest E-bit is always off */
	/* Encode command/response in C bits */
	switch(hdr->cmdrsp){
	case COMMAND:
		hdr->dest.ssid |= C;
		hdr->source.ssid &= ~C;
		break;
	case RESPONSE:
		hdr->dest.ssid &= ~C;
		hdr->source.ssid |= C;
		break;
	default:
		hdr->dest.ssid &= ~C;
		hdr->source.ssid &= ~C;
		break;
	}
	cp = putaxaddr(cp,&hdr->dest);

	/* Set E bit on source address if no digis */
	if(hdr->ndigis == 0){
		hdr->source.ssid |= E;
		putaxaddr(cp,&hdr->source);
		return bp;
	}
	hdr->source.ssid &= ~E;
	cp = putaxaddr(cp,&hdr->source);

	/* All but last digi get copied with E bit off */
	for(i=0; i < hdr->ndigis - 1; i++){
		hdr->digis[i].ssid &= ~E;
		cp = putaxaddr(cp,&hdr->digis[i]);
	}
	hdr->digis[i].ssid |= E;
	cp = putaxaddr(cp,&hdr->digis[i]);
	return bp;
}


/* Convert a network-format AX.25 header into a host format structure
 * Return -1 if error, number of addresses if OK
 */
int
ntohax25(struct ax25 *hdr, struct mbuf **bpp)
{
	register struct ax25_addr *axp;
	char buf[AXALEN];

	if(pullup(bpp, buf, AXALEN) < AXALEN)
		return -1;
	getaxaddr(&hdr->dest, buf);

	if(pullup(bpp, buf, AXALEN) < AXALEN)
		return -1;
	getaxaddr(&hdr->source, buf);

			/* Process C bits to get command/response indication */

	if((hdr->source.ssid & C) == (hdr->dest.ssid & C))
		hdr->cmdrsp = UNKNOWN;
	else if(hdr->source.ssid & C)
		hdr->cmdrsp = RESPONSE;
	else
		hdr->cmdrsp = COMMAND;

	hdr->ndigis = 0;
	if(hdr->source.ssid & E)
		return 2;	/* No digis */

			/* Process digipeaters */

	for(axp=hdr->digis; axp<&hdr->digis[MAXDIGIS]; axp++) {
		if(pullup(bpp, buf, AXALEN) < AXALEN)
			return -1;
		getaxaddr(axp, buf);
		if(axp->ssid & E){	/* Last one */
			hdr->ndigis = axp - hdr->digis + 1;
			return hdr->ndigis + 2;			
		}
	}
	return -1;	/* Too many digis */
}

/* Figure out the frame type from the control field
 * This is done by masking out any sequence numbers and the
 * poll/final bit after determining the general class (I/S/U) of the frame
 */
int
ftype(char control)
{
	if((control & 1) == 0)	/* An I-frame is an I-frame... */
		return I;
	if(control & 2)		/* U-frames use all except P/F bit for type */
		return(control & ~PF);
	else			/* S-frames use low order 4 bits for type */
		return(control & 0xf);
}

