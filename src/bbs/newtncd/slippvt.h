#ifndef	_SLIPPVT_H
#define	_SLIPPVT_H

#ifndef	_GLOBAL_H
#include "global.h"
#endif

#ifndef _IFACE_H
#include "iface.h"
#endif

#define SLIP_MAX 6		/* Maximum number of slip channels */

/* Slip protocol control structure */
struct slip {
	struct iface *iface;
	uint8 escaped;		/* Receiver State control flag */
#define SLIP_FLAG	0x01		/* Last char was a frame escape */
	struct mbuf *rbp_head;	/* Head of mbuf chain being filled */
	struct mbuf *rbp_tail;	/* Pointer to mbuf currently being written */
	uint8 *rcp;		/* Write pointer */
	uint rcnt;		/* Length of mbuf chain */
	struct mbuf *tbp;	/* Transmit mbuf being sent */
	uint errors;		/* Receiver input errors */
	int type;		/* Protocol of input */
	int (*send)(int,struct mbuf **);	/* send mbufs to device */
};

/* In slip.c: */
extern struct slip Slip[];

#endif	/* _SLIP_H */
