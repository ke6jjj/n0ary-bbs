#ifndef	_MBUF_H
#define	_MBUF_H

#include "stdio.h"

#ifndef _GLOBAL_H
#include "global.h"
#endif

extern unsigned Ibufsize;	/* Size of interrupt buffers to allocate */
extern int Nibufs;		/* Number of interrupt buffers to allocate */

/* Basic message buffer structure */
struct mbuf {
	struct mbuf *next;	/* Links mbufs belonging to single packets */
	struct mbuf *anext;	/* Links packets on queues */
	uint size;		/* Size of associated data buffer */
	int refcnt;		/* Reference count */
	struct mbuf *dup;	/* Pointer to duplicated mbuf */
	uint8 *data;		/* Active working pointers */
	uint cnt;
};

#define	PULLCHAR(bpp)\
 ((bpp) != NULL && (*bpp) != NULL && (*bpp)->cnt > 1 ? \
 ((*bpp)->cnt--,*(*bpp)->data++) : pullchar(bpp))

/* In mbuf.c: */
struct mbuf *alloc_mbuf(uint size);
void free_mbuf(struct mbuf **bpp);

struct mbuf *ambufw(uint size);
struct mbuf *copy_p(struct mbuf *bp,uint cnt);
uint dup_p(struct mbuf **hp,struct mbuf *bp,uint offset,uint cnt);
uint extract(struct mbuf *bp,uint offset,void *buf,uint len);
struct mbuf *free_p(struct mbuf **bpp);
uint len_p(struct mbuf *bp);
void trim_mbuf(struct mbuf **bpp,uint length);
int write_p(kFILE *fp,struct mbuf *bp);

struct mbuf *dequeue(struct mbuf **q);
void enqueue(struct mbuf **q,struct mbuf **bpp);
void free_q(struct mbuf **q);
uint len_q(struct mbuf *bp);

struct mbuf *qdata(const void *data,uint cnt);
uint dqdata(struct mbuf *bp,void *buf,unsigned cnt);

void append(struct mbuf **bph,struct mbuf **bpp);
void pushdown(struct mbuf **bpp,void *buf,uint size);
uint pullup(struct mbuf **bph,void *buf,uint cnt);

#define	pullchar(x) pull8(x)
int pull8(struct mbuf **bpp);       /* returns -1 if nothing */
long pull16(struct mbuf **bpp);	/* returns -1 if nothing */
int32 pull32(struct mbuf **bpp);	/* returns  0 if nothing */

uint get16(uint8 *cp);
int32 get32(uint8 *cp);
uint8 *put16(uint8 *cp,uint x);
uint8 *put32(uint8 *cp,int32 x);

void iqstat(void);
void refiq(void);
void mbuf_crunch(struct mbuf **bpp);

void mbufsizes(void);
void mbufstat(void);
void mbuf_garbage(int red);

#define AUDIT(bp)       audit(bp,__FILE__,__LINE__)

#endif	/* _MBUF_H */
