/*
 * A method for simulating process wakeup signaling using the features of
 * the alib asynchronous library.
 */
#ifndef ASIGNAL_H
#define ASIGNAL_H

#include "alib.h"

/*
 * Register a callback to be called when any signal to "addr" is made. A
 * signal is accompanied by an integer signal number to be sent (signum).
 * The specified callback function will be called and given the following
 * arguments:
 *
 *  cb.fb(cb.obj, NULL, signum)
 *
 * No provision is made to deduplicate entries in the system. If the same
 * callback is registered more than once, it will be called more than once.
 *
 * Returns 0 on success, -1 on failure.
 */
int asignal_register(const void *addr, alCallback cb);

/*
 * Deregisters a previously registered signal callback.
 *
 * Returns 0 if deregistered, -1 if not found.
 */
int asignal_deregister(const void *addr, alCallback cb);

/*
 * Call all registered callback functions for an address, giving them signum.
 *
 * Returns 0 if at least one callback was made, -1 if none.
 */
int asignal(const void *addr, int signum);

int asignal_init(void);
int asignal_deinit(void);

#endif
