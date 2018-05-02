#ifndef _TNCD_KISS_MUX_H
#define _TNCD_KISS_MUX_H

#include "kiss.h"
#include "slip.h"

int kiss_mux_init(kiss *upkiss, slip *downslip, int see_others,
	char *bind_addr, int bind_port);
int kiss_mux_shutdown(void);

int kiss_nexus_recv(void *up_kiss, struct mbuf *bp);
int kiss_nexus_send(void *down_slip, struct mbuf *bp);

#endif
