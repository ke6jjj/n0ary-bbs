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
};
extern struct slip slip[];

int slip_start(int dev);
int slip_stop(int dev);

extern int Tncd_SLIP_Flags;

#define SLIP_FLAGS_ESCAPE_ASCII_C 0x1

extern int
	slip_raw(int dev, struct mbuf *data),
	slip_init(int dev),
	slip_set_flags(int dev, int flags);
