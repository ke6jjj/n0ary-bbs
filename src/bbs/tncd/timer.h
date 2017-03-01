/* Software timers
 * There is one of these structures for each simulated timer.
 * Whenever the timer is running, it is on a doubly-linked list
 * pointed to by "timers" so that the (hardware) timer interrupt
 * can quickly run through the list and change counts and states.
 * Stopping a timer or letting it expire causes it to be removed
 * from the list; starting a timer puts it on the list.
 */
struct timer {
	struct timer *next;	/* Doubly-linked-list pointers */
	struct timer *prev;
	int start;		/* Period of counter (load value) */
	int count;		/* Ticks to go until expiration */
	void (*func)();		/* Function to call at expiration */
	char *arg;		/* Arg to pass function */
	char state;		/* Timer state */
#define	TIMER_STOP	0
#define	TIMER_RUN	1
#define	TIMER_EXPIRE	2
};

#define	NULLTIMER	(struct timer *)0
#define	MAX_TIME	(int)4294967295	/* Max long integer */

#ifndef MSPTICK  /* { */
#define	MSPTICK		55		/* Milliseconds per tick */
#endif   /* } */

/* Useful user macros that hide the timer structure internals */
#define	run_timer(t)	((t)->state == TIMER_RUN)

extern void
	start_timer(struct timer *t),
	stop_timer(struct timer *t),
	tick(void);
