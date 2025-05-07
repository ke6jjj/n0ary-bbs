#ifndef _TNCD_KISS_H
#define _TNCD_KISS_H

#include <stdio.h>

#include "calls.h"

typedef struct kiss kiss;

kiss *kiss_init(void);
int kiss_set_pcap_dump(kiss *dev, FILE *fp);
int kiss_raw(kiss *dev, struct mbuf *data);
int kiss_recv(void *dev, struct mbuf *bp);
int kiss_set_send(kiss *dev, mbuf_recv_fn send_fn, void *send_arg);
void kiss_deinit(kiss *dev);

#endif
