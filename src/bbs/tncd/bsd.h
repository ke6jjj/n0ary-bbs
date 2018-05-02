#ifndef _ASY_H
#define _ASY_H

#include "calls.h"

void Tncd_TX_Enable(int enable);
int Is_Tncd_TX_Enabled(void);
extern int Tncd_SLIP_Flags;

typedef struct asy asy;

typedef void (*asy_notify_fn)(void *arg, asy *asy, int is_read, int error);

/* Initializers */
asy *asy_init(char *ttydev);
asy *asy_init_from_fd(int fd);

/* Set upcall for received data */
int asy_set_recv(asy *asy, buf_recv_fn recv, void *arg);

/* Set error notification function */
int asy_set_notify(asy *asy, asy_notify_fn fn, void *arg);

/* Transmit an mbuf on the line */
int asy_send(void *asy, struct mbuf *mb);

/* Temporarily enable/disable transmittion (dropping data if disabled */
int asy_enable(asy *asy, int enable);
int asy_enabled(asy *asy);

/* Start working */
int asy_start(asy *asy);

/* Stop working */
int asy_stop(asy *asy);

/* Deallocate */
int asy_deinit(asy *asy);

#endif
