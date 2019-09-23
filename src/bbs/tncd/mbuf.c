/* Primitive mbuf allocate/free routines */
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include "c_cmmn.h"
#include "global.h"
#include "mbuf.h"

/* Allocate mbuf with associated buffer of 'size' bytes */
struct mbuf *
alloc_mbuf(int size)
{
	register struct mbuf *bp;

	if((bp = (struct mbuf *)malloc((unsigned)(size + sizeof(struct mbuf)))) == NULLBUF)
		return NULLBUF;
	bp->next = bp->anext = NULLBUF;
	if(size != 0){
		bp->data = (unsigned char *)(bp + 1);
	} else {
		bp->data = NULL;
	}
	bp->size = size;
	bp->cnt = 0;
	return bp;
}

/* Free all resources associated with mbuf
 * Return pointer to next mbuf in packet chain
 */
struct mbuf *
free_mbuf(struct mbuf *bp)
{
	register struct mbuf *bp1 = NULLBUF;

	if(bp != NULLBUF){
		bp1 = bp->next;
		bp->next = NULLBUF;	/* detect attempts to use */
		bp->data = NULL;	/* a freed mbuf */
		free((char *)bp);
	}
	return bp1;
}

/* Free packet (a chain of mbufs). Return pointer to next packet on queue,
 * if any
 */
struct mbuf *
free_p(struct mbuf *bp)
{
	struct mbuf *abp;

	if(bp == NULLBUF)
		return NULLBUF;
	abp = bp->anext;
	while(bp != NULLBUF)
		bp = free_mbuf(bp);
	return abp;
}		
/* Free entire queue of packets (of mbufs) */
void
free_q(struct mbuf **q)
{
	register struct mbuf *bp;

	while((bp = dequeue(q)) != NULLBUF)
		free_p(bp);
}

/* Count up the total number of bytes in an mbuf */
int
len_mbuf(struct mbuf *bp)
{
	int cnt;

	cnt = 0;
	while(bp != NULLBUF){
		cnt += bp->cnt;
		bp = bp->next;
	}
	return cnt;
}

/* Duplicate/enqueue/dequeue operations based on mbufs */

/* Duplicate first 'cnt' bytes of packet starting at 'offset'.
 * This is done without copying data; only the headers are duplicated,
 * but without data segments of their own. The pointers are set up to
 * share the data segments of the original copy. The return pointer is
 * passed back through the first argument, and the return value is the
 * number of bytes actually duplicated.
 */
int
dup_p(struct mbuf **hp, struct mbuf *bp, int offset, int cnt)
{
	struct mbuf *cp;
	int tot;

	if(cnt == 0 || bp == NULLBUF || hp == NULLBUFP){
		if(hp != NULLBUFP)
			*hp = NULLBUF;
		return 0;
	}
	if((*hp = cp = alloc_mbuf(0)) == NULLBUF){
		return 0;
	}
	/* Skip over leading mbufs that are smaller than the offset */
	while(bp != NULLBUF && bp->cnt <= offset){
		offset -= bp->cnt;
		bp = bp->next;
	}
	if(bp == NULLBUF){
		free_mbuf(cp);
		*hp = NULLBUF;
		return 0;	/* Offset was too big */
	}
	tot = 0;
	for(;;){
		cp->data = bp->data + offset;
		cp->cnt = min(cnt,bp->cnt - offset);
		offset = 0;
		cnt -= cp->cnt;
		tot += cp->cnt;
		bp = bp->next;
		if(cnt == 0 || bp == NULLBUF || (cp->next = alloc_mbuf(0)) == NULLBUF)
			break;
		cp = cp->next;
	}
	return tot;
}

/* Copy and delete "cnt" bytes from beginning of packet. Return number of
 * bytes actually pulled off
 */
int
pullup(struct mbuf **bph, char *buf, int cnt)
{
	register struct mbuf *bp;
	int n,tot;

	tot = 0;
	if(bph == NULLBUFP)
		return 0;
	while(*bph != NULLBUF && cnt != 0){
		bp = *bph;
		n = min(cnt,bp->cnt);
		if(buf != NULL&& n != 0){
			memcpy(buf,bp->data,n);
			buf += n;
		}
		tot += n;
		cnt -= n;
		bp->data += n;
		bp->cnt -= n;		
		if(bp->cnt == 0){
			*bph = free_mbuf(bp);
		}
	}
	return tot;
}
/* Append mbuf to end of mbuf chain */
void
append(struct mbuf **bph, struct mbuf *bp)
{
	if(bph == NULLBUFP || bp == NULLBUF)
		return;
	if(*bph == NULLBUF){
		/* First one on chain */
		*bph = bp;
	} else {
		struct mbuf *p = *bph;
		while(p->next != NULLBUF)
			NEXT(p);
		p->next = bp;
	}
}
/* Insert specified amount of contiguous new space at the beginning of an
 * mbuf chain. If enough space is available in the first mbuf, no new space
 * is allocated. Otherwise a mbuf of the appropriate size is allocated and
 * tacked on the front of the chain.
 *
 * This operation is the logical inverse of pullup(), hence the name.
 */
struct mbuf *
pushdown(struct mbuf *bp, int size)
{
	register struct mbuf *nbp;

	/* Check that bp is real and that there's data space associated with
	 * this buffer (i.e., this is not a buffer from dup_p) before
	 * checking to see if there's enough space at its front
	 */
	if(bp != NULLBUF && bp->size != 0 && bp->data - (unsigned char *)(bp+1) >= size){
		/* No need to alloc new mbuf, just adjust this one */
		bp->data -= size;
		bp->cnt += size;
	} else {
		if((nbp = alloc_mbuf(size)) != NULLBUF){
			nbp->next = bp;
			nbp->cnt = size;
			bp = nbp;
		} else {
			bp = NULLBUF;
		}
	}
	return bp;
}
/* Append packet to end of packet queue */
void
enqueue(struct mbuf **q, struct mbuf *bp)
{
	if(q == NULLBUFP || bp == NULLBUF)
		return;
	if(*q == NULLBUF){
		/* List is empty, stick at front */
		*q = bp;
	} else {
		struct mbuf *p = *q;
		while(p->anext != NULLBUF)
			p = p->anext;
		p->anext = bp;
	}
}

/* Unlink a packet from the head of the queue */
struct mbuf *
dequeue(struct mbuf **q)
{
	struct mbuf *bp;

	if(q == NULLBUFP)
		return NULLBUF;

	if((bp = *q) != NULLBUF){
		*q = bp->anext;
		bp->anext = NULLBUF;
	}
	return bp;
}	

/* Pull single character from mbuf */
char
pullchar(struct mbuf **bpp)
{
	char c;

	if(pullup(bpp,&c,1) != 1)
		/* Return zero if nothing left */
		c = 0;
	return c;
}
