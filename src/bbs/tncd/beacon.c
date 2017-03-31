#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "alib.h"
#include "ax25.h"
#include "beacon.h"
#include "c_cmmn.h"

struct beacon_task {
	struct	  ax25_cb axcb; /* Pre-built packet header */
	int       dev;
	char     *text;
	size_t    textlen;
	int       timer;
	int	  repeat_time_ms;
	alCallback jitter_cb;
	alCallback beacon_cb;
};

static void beacon_jitter_done(void *obj, void *arg0, int arg1);
static void beacon_now(void *obj, void *arg0, int arg1);

beacon_task *
beacon_task_new(const char *from_callsign, const char *to_callsign,
	const char *text, int repeat_time_s, int dev)
{
	beacon_task *task = alMalloc(beacon_task, 1);
	if (task == NULL)
		return NULL;

	task->text = strdup(text);
	if (task->text == NULL)
		goto TextAllocFailed;

	setcall(&task->axcb.addr.dest, to_callsign);
	setcall(&task->axcb.addr.source, from_callsign);
	task->axcb.addr.ndigis = 0;
	task->axcb.addr.cmdrsp = UNKNOWN;
	task->axcb.dev = dev;

	task->textlen = strlen(text);
	task->timer = -1;
	task->repeat_time_ms = repeat_time_s * 1000;

	AL_CALLBACK(&task->beacon_cb, task, beacon_now);
	AL_CALLBACK(&task->jitter_cb, task, beacon_jitter_done);

	return task;

TextAllocFailed:
	free(task);
	return NULL;
}

int
beacon_task_start(beacon_task *task, int jitter_s)
{
	/* Queue a small jitter timer before really starting */
	task->timer = alEvent_addTimer(jitter_s*1000, 0, task->jitter_cb);
	if (task->timer == -1) {
		fprintf(stderr, "beacon jitter arm failed\n");
		goto ArmTimerFailed;
	}

	return 0;

ArmTimerFailed:
	return -1;
}

/* The initial jitter timer has expired. Start regular beaconing */
static void
beacon_jitter_done(void *obj, void *arg0, int arg1)
{
	beacon_task *task = obj;

	assert(arg1 == task->timer);

	/* Queue the regular beacon up */
	task->timer = alEvent_addTimer(task->repeat_time_ms,
		ALTIMER_REPEAT, task->beacon_cb);
	if (task->timer == -1) {
		fprintf(stderr, "beacon arm failed\n");
	}
}

static void
beacon_now(void *obj, void *arg0, int arg1)
{
	beacon_task *task = obj;
	struct mbuf *bp;
	alCallback cb;

	assert(arg1 == task->timer);

	/* Allocate an mbuf to hold the beacon text plus protocol id */
	bp = alloc_mbuf(task->textlen + 1);
	if (bp == NULL) {
		fprintf(stderr, "beacon alloc mbuf failed\n");
		goto MBufAllocFailed;
	}

	bp->cnt = task->textlen + 1;
	bp->data[0] = PID_FIRST | PID_LAST | PID_NO_L3;
	memcpy(&bp->data[1], task->text, task->textlen);

	if (sendframe(&task->axcb, UNKNOWN, UI, bp) == ERROR)
		fprintf(stderr, "beacon send failed\n");

MBufAllocFailed:
	return;
}

void
beacon_task_stop(beacon_task *task)
{
	if (task->timer != -1) {
		alEvent_cancelTimer(task->timer);
		task->timer = -1;
	}
}

void
beacon_task_free(beacon_task * task)
{
	free(task->text);
	free(task);
}

