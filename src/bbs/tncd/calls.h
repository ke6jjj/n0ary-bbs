#ifndef _TNC_CALLS_H
#define _TNC_CALLS_H

#include <stddef.h>
#include "mbuf.h"

/* A function receiving an incoming mbuf */
typedef int (*mbuf_recv_fn)(void *ctx, struct mbuf *bp);

/* A function receiving a flat data buffer */
typedef int (*buf_recv_fn)(void *ctx, const char *data, size_t count);

#endif
