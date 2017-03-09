#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>

#include "alib.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "global.h"
#include "timer.h"

static void timer_fired(void *obj, void *arg0, int arg1);

int
init_timer(struct timer *t)
{
	t->al_timerid = -1;
	t->state = TIMER_STOP;

	return 0;
}

int
start_timer(struct timer *t)
{
	alCallback cb;

	if (t == NULL || t->start == 0)
		return -1;

	if (t->state == TIMER_RUN) {
		/* Timer is already running */
		assert(t->al_timerid != -1);
		alEvent_cancelTimer(t->al_timerid);
		t->al_timerid = -1;
		t->state = TIMER_STOP;
	}

	AL_CALLBACK(&cb, t, timer_fired);
	t->al_timerid = alEvent_addTimer(t->start * MSPTICK, 0, cb);
	assert(t->al_timerid != -1);
	t->state = TIMER_RUN;

	return 0;
}

static void
timer_fired(void *obj, void *arg0, int arg1)
{
	struct timer *t = obj;

	assert(t->state == TIMER_RUN);
	t->state = TIMER_EXPIRE;
	t->al_timerid = -1;

	if (t->func)
		(*t->func)(t->arg);
}

/* Stop a timer */
int
stop_timer(struct timer *t)
{
	if(t == NULL)
		return -1;

	if(t->state == TIMER_RUN){
		assert(t->al_timerid != -1);
		alEvent_cancelTimer(t->al_timerid);
		t->al_timerid = -1;
	}

	t->state = TIMER_STOP;

	return 0;
}
