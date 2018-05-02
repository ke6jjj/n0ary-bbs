#ifndef BEACON_TASK_H
#define BEACON_TASK_H

#include "kiss.h"

struct beacon_task;
typedef struct beacon_task beacon_task;

beacon_task *beacon_task_new(const char *from_callsign, const char *to_calsign,
	const char *text, int repeat_time, kiss *dev);
int beacon_task_start(beacon_task *task, int jitter_s);
void beacon_task_stop(beacon_task *task);
void beacon_task_free(beacon_task * task);

#endif
