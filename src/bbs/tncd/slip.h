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
	char escaped;		/* Receiver State control flag */
	struct mbuf *rbp;	/* Head of mbuf chain being filled */
	struct mbuf *rbp1;	/* Pointer to mbuf currently being written */
	char *rcp;		/* Write pointer */
	int rcnt;		/* Length of mbuf chain */
	struct mbuf *tbp;	/* Transmit mbuf being sent */
	int errors;		/* Receiver input errors */
	void (*recv)();		/* Function to call with an incoming buffer */
};
extern struct slip slip[];

extern void
	doslip(int dev);

extern int
	slip_raw(int dev, struct mbuf *data);
