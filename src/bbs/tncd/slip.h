#ifndef _TNCD_SLIP_H
#define _TNCD_SLIP_H

#include "calls.h"

#define SLIP_FLAGS_ESCAPE_ASCII_C 0x1

typedef struct slip slip;

slip *slip_init(int flags);
int slip_set_send(slip *sp, mbuf_recv_fn send, void *arg);
int slip_set_recv(slip *sp, mbuf_recv_fn recv, void *arg);

/* Process SLIP line input */
int slip_input(void *sp, const char *buf, size_t sz);

/* Send mbuf on SLIP line output */
int slip_output(void *sp, struct mbuf *data);

void slip_deinit(slip *sp);

#endif
