/* KISS TNC control */
#define	KISS_DATA	0

extern int
	kiss_raw(int dev, struct mbuf *data);

extern void
	kiss_recv(int dev, struct mbuf *bp);
