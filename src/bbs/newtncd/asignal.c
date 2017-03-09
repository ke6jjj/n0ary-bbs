/*
 * A method for simulating process wakeup signaling using the features of
 * the alib asynchronous library.
 *
 * Uses a hash table for indexing.
 */
#include <sys/queue.h>
#include <stdlib.h>

#include "asignal.h"

/* The hashtable size */
#define SIGNAL_HASHVAL 117

/* A single entry in the signal table */
typedef struct callback_entry {
	const void *addr; /* The address/key to look for */
	alCallback cb;    /* The callback function (plus object) */
	LIST_ENTRY(callback_entry) listEntry; /* List linkage */
} callback_entry;

/* The global signal hashtable */
static LIST_HEAD(, callback_entry) gCallbacks[SIGNAL_HASHVAL];

/* Initialize the signaling system. */
int
asignal_init(void)
{
	size_t i;

	for (i = 0; i < SIGNAL_HASHVAL; i++)
		LIST_INIT(&gCallbacks[i]);

	return 0;
}

/* Teardown the signaling system */
int
asignal_deinit(void)
{
	size_t i;
	callback_entry *cb;

	for (i = 0; i < SIGNAL_HASHVAL; i++) {
		while (!LIST_EMPTY(&gCallbacks[i])) {
			cb = LIST_FIRST(&gCallbacks[i]);
			LIST_REMOVE(cb, listEntry);
			free(cb);
		}
	}

	return 0;
}

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
int
asignal_register(const void *addr, alCallback cb)
{
	callback_entry *cbe;
	size_t i;

	cbe = alMalloc(callback_entry, 1);
	if (cbe == NULL)
		return -1;

	cbe->addr = addr;
	cbe->cb = cb;
	i = (const unsigned int)addr % SIGNAL_HASHVAL;

	LIST_INSERT_HEAD(&gCallbacks[i], cbe, listEntry);

	return 0;
}

/*
 * Deregisters a previously registered signal callback.
 *
 * Returns 0 if deregistered, -1 if not found.
 */
int
asignal_deregister(const void *addr, alCallback cb)
{
	callback_entry *cbe;
	size_t i;

	i = (const unsigned int)addr % SIGNAL_HASHVAL;
	LIST_FOREACH(cbe, &gCallbacks[i], listEntry) {
		if (cbe->addr == addr && AL_CALLBACK_EQUAL(&cbe->cb, &cb)) {
			/* Found. */
			LIST_REMOVE(cbe, listEntry);
			free(cbe);
			return 0;
		}
	}

	return -1;
}

/*
 * Call all registered callback functions for an address, giving them signum
 * as an argument in the integer position (arg1).
 *
 * Returns 0 if at least one callback was made, -1 if none.
 */
int
asignal(const void *addr, int signum)
{
	callback_entry *cbe;
	size_t i;
	int foundone;

	foundone = 0;
	i = (const unsigned int)addr % SIGNAL_HASHVAL;
	LIST_FOREACH(cbe, &gCallbacks[i], listEntry) {
		if (cbe->addr == addr) {
			foundone = 1;
			alEvent_doCallback(cbe->cb, NULL, signum);
		}
	}

	return foundone ? 0 : -1;
}
