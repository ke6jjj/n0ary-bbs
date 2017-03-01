/* AX.25 datagram (address) sub-layer definitions */

/* Maximum number of digipeaters */
#define	MAXDIGIS	7	/* 7 digipeaters plus src/dest */
#define	ALEN		6	/* Number of chars in callsign field */
#define	AXALEN		7	/* Total AX.25 address length, including SSID */

/* Internal representation of an AX.25 address */
struct ax25_addr {
	unsigned char call[ALEN];	
	unsigned char ssid;
#define	SSID		0x1e	/* Sub station ID */
#define	REPEATED	0x80	/* Has-been-repeated bit in repeater field */
#define	E		0x01	/* Address extension bit */
#define	C		0x80	/* Command/response designation */
};
#define	NULLAXADDR	(struct ax25_addr *)0
/* Our AX.25 address */
extern struct ax25_addr mycall;
extern struct ax25_addr bbscall, fwdcall;

struct ax25_addr_list {
	struct ax25_addr_list *next;
	struct ax25_addr *addr;
};
extern struct ax25_addr_list *CallList;

/* AX.25 broadcast address: "QST   -0" in shifted ASCII */
extern struct ax25_addr ax25_bdcst;

/* Internal representation of an AX.25 header */
struct ax25 {
	struct ax25_addr dest;			/* Destination address */
	struct ax25_addr source;		/* Source address */
	struct ax25_addr digis[MAXDIGIS];	/* Digi string */
	int ndigis;				/* Number of digipeaters */
	int cmdrsp;				/* Command/response */
};

/* C-bit stuff */
#define	UNKNOWN		0
#define	COMMAND		1
#define	RESPONSE	2

/* Bit fields in AX.25 Level 3 Protocol IDs (PIDs)
 * The high order two bits control multi-frame messages.
 * The lower 6 bits is the actual PID. Single-frame messages are
 * sent with both the FIRST and LAST bits set, so that the resulting PIDs
 * are compatible with older code.
 */
#define	PID_FIRST	0x80	/* Frame is first in a message */
#define	PID_LAST	0x40	/* Frame is last in a message */
#define	PID_PID		0x3f	/* Protocol ID subfield */

#define	PID_IP		0x0c	/* ARPA Internet Protocol */
#define	PID_ARP		0x0d	/* ARPA Address Resolution Protocol */
#define	PID_NETROM	0x0f	/* NET/ROM */
#define	PID_NO_L3	0x30	/* No level 3 protocol */

/* Upper sub-layer (LAPB) definitions */

/* Control field templates */
#define	I	0x00	/* Information frames */
#define	S	0x01	/* Supervisory frames */
#define	RR	0x01	/* Receiver ready */
#define	RNR	0x05	/* Receiver not ready */
#define	REJ	0x09	/* Reject */
#define	U	0x03	/* Unnumbered frames */
#define	SABM	0x2f	/* Set Asynchronous Balanced Mode */
#define	DISC	0x43	/* Disconnect */
#define	DM	0x0f	/* Disconnected mode */
#define	UA	0x63	/* Unnumbered acknowledge */
#define	FRMR	0x87	/* Frame reject */
#define	UI	0x03	/* Unnumbered information */
#define	PF	0x10	/* Poll/final bit */

#define	MMASK	7	/* Mask for modulo-8 sequence numbers */

/* FRMR reason bits */
#define	W	1	/* Invalid control field */
#define	X	2	/* Unallowed I-field */
#define	Y	4	/* Too-long I-field */
#define	Z	8	/* Invalid sequence number */

/* Per-connection link control block
 * These are created and destroyed dynamically,
 * and are indexed through a hash table.
 * One exists for each logical AX.25 Level 2 connection
 */
struct ax25_cb {
	struct ax25_cb *next;		/* Doubly linked list pointers */
	struct ax25_cb *prev;

	struct mbuf *txq;		/* Transmit queue */
	struct mbuf *rxasm;		/* Receive reassembly buffer */
	struct mbuf *rxq;		/* Receive queue */

	struct ax25 addr;		/* Address header */

	int dev;

	char rejsent;			/* REJ frame has been sent */
	char remotebusy;		/* Remote sent RNR */
	char response;			/* Response owed to other end */

	char vs;			/* Our send state variable */
	char vr;			/* Our receive state variable */
	char unack;			/* Number of unacked frames */
	int maxframe;			/* Transmit flow control level */
	int paclen;			/* Maximum outbound packet size */
	int window;			/* Local flow control limit */
	char proto;			/* Protocol version */
#define	V1	1			/* AX.25 Version 1 */
#define	V2	2			/* AX.25 Version 2 */
	int	pthresh;		/* Poll threshold */
	unsigned retries;		/* Retry counter */
	unsigned n2;			/* Retry limit */
	int state;			/* Link state */
#define	DISCONNECTED	0
#define	SETUP		1
#define	DISCPENDING	2
#define	CONNECTED	3
#define	RECOVERY	4
#define	FRAMEREJECT	5
	char frmrinfo[3];		/* I-field for FRMR message */
	struct timer t1;		/* Retry timer */
	struct timer t2;		/* Acknowledgement delay timer */
	struct timer t3;		/* Keep-alive poll timer */

	void (*r_upcall)();		/* Receiver upcall */
	void (*t_upcall)();		/* Transmit upcall */
	void (*s_upcall)();		/* State change upcall */
	char *user;			/* User pointer */
};

#define	NULLAX25	((struct ax25_cb *)0)
extern struct ax25_cb ax25default;
extern struct ax25_cb *ax25_cb[];
#define	NHASH	17

#undef YES
#undef NO

#define	YES	1
#define	NO	0

extern int Tncd_T1init;
extern int Tncd_T2init;
extern int Tncd_T3init;
extern int Tncd_Maxframe;
extern int Tncd_N2;
extern int Tncd_Paclen;
extern int Tncd_Pthresh;

extern int Tncd_Control_Port, Tncd_Monitor_Port, monitor_socket;
extern char *Tncd_Host;
extern char *Tncd_Name;
extern char *Tncd_Device;

extern int
	lapb_output(struct ax25_cb *axp),
	sendctl(struct ax25_cb *axp, char cmdrsp,char cmd),
	frmr(struct ax25_cb *axp, char control, char reason),
	lapb_input(struct ax25_cb *axp, char cmdrsp, struct mbuf *bp);

extern void
	recover(int *n),
	pollthem(int *n),
	send_ack(int *n),
	lapbstate(struct ax25_cb *axp, int s),
	est_link(struct ax25_cb *axp);
extern void
	ax_recv(int dev, struct mbuf *bp),
	ax25_dump(struct mbuf *bp),
	disc_ax25(struct ax25_cb *axp),
	pax25(char *e, struct ax25_addr *addr),
	add_call(struct ax25_addr *a),
	delete_call(struct ax25_addr *a),
	del_ax25(struct ax25_cb *axp),
	build_bbscall(void);

extern int
	send_ax25(struct ax25_cb *axp, struct mbuf *bp),
	ntohax25(struct ax25 *hdr, struct mbuf **bpp),
	ftype(char control),
	scan_calls(struct ax25_addr *my_addr, struct ax25_addr *their_addr),
	addreq_sid(struct ax25_addr *a, struct ax25_addr *b),
	addreq(struct ax25_addr *a, struct ax25_addr *b),
	setcall(struct ax25_addr *out, char *call),
	sendframe(struct ax25_cb *axp, char cmdrsp, char ctl, struct mbuf *data);

extern struct ax25_cb
	*open_ax25(struct ax25 *addr, int window,
		void (*r_upcall)(), void (*t_upcall)(), void (*s_upcall)(),
		int dev, char *user),
	*cr_ax25(int dev, struct ax25_addr *my_addr, struct ax25_addr *their_addr),
	*find_ax25(struct ax25_addr *my_addr, struct ax25_addr *their_addr);

extern char
	*getaxaddr(struct ax25_addr *ap, char *cp);

extern struct mbuf
	*recv_ax25(struct ax25_cb *axp),
	*htonax25(struct ax25 *hdr, struct mbuf *data);

