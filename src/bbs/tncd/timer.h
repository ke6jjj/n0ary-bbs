#include <sys/time.h>

#include "alib.h"

/* Software timers
 *
 * There is one of these structures for each simulated timer.
 *
 * Whenever the timer is running, it is on a sorted, doubly-linked list
 * pointed to by "timers" so that the timer checking routine
 * can quickly run through the list and check for those timers that have
 * expired.
 *
 * Additionally, the application event processing loop can quickly ask
 * for the first timer that is due to expire and plan accordingly.
 *
 * Stopping a timer or letting it expire causes it to be removed
 * from the list; starting a timer puts it on the list.
 */
struct timer {
	int start;		/* Period of counter (load value) */
	void (*func)();		/* Function to call at expiration */
	char *arg;		/* Arg to pass function */
	char state;		/* Timer state */
	int al_timerid;         /* Alib timer identifier (if running) */
#define	TIMER_STOP	0
#define	TIMER_RUN	1
#define	TIMER_EXPIRE	2
};

#ifndef MSPTICK  /* { */
#define	MSPTICK		1000		/* Milliseconds per tick */
#endif   /* } */

/* Useful user macros that hide the timer structure internals */
#define	run_timer(t)	((t)->state == TIMER_RUN)

int init_timer(struct timer *t);
int start_timer(struct timer *t);
int stop_timer(struct timer *t);
