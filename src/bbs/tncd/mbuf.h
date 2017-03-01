/* Basic message buffer structure */
struct mbuf {
	struct mbuf *next;		/* Links mbufs belonging to single packets */
	struct mbuf *anext;		/* Links packets on queues */
	int size;				/* Size of associated data buffer */
	char *data;				/* Active working pointers */
	int cnt;
};

#define	NULLBUF	(struct mbuf *)0
#define	NULLBUFP (struct mbuf **)0

extern char
	pullchar(struct mbuf **bpp);

extern void
	append(struct mbuf **bph, struct mbuf *bp),
	enqueue(struct mbuf **q, struct mbuf *bp),
	free_q(struct mbuf **q);

extern int
	pullup(struct mbuf **bph, char *buf, int cnt),
	dup_p(struct mbuf **hp, struct mbuf *bp, int offset, int cnt),
	len_mbuf(struct mbuf *bp);

extern struct mbuf
	*dequeue(struct mbuf **q),
	*pushdown(struct mbuf *bp, int size),
	*free_p(struct mbuf *bp),
	*free_mbuf(struct mbuf *bp),
	*alloc_mbuf(int size);
