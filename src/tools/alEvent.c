/*
 * Copyright 2017 Jeremy Cooper. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Jeremy Cooper.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/event.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "alib.h"

/*****************************************************************************
 * PRIVATE MACROS                                                            *
 *****************************************************************************/

/*
 * Handy timespec manipulation functions.
 */
#define timespecadd(tsp, usp, vsp)                                      \
        do {                                                            \
                (vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;          \
                (vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec;       \
                if ((vvp)->tv_ssec >= 1000000000) {                     \
                        (vvp)->tv_sec++;                                \
                        (vvp)->tv_nsec -= 1000000000;                   \
                }                                                       \
        } while (0)
#define timespecsub(tsp, usp, vsp)                                      \
        do {                                                            \
                (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;          \
                (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;       \
                if ((vsp)->tv_nsec < 0) {                               \
                        (vsp)->tv_sec--;                                \
                        (vsp)->tv_nsec += 1000000000;                   \
                }                                                       \
        } while (0)
#define timespeccmp(tsp, usp, cmp)                                      \
        (((tsp)->tv_sec == (usp)->tv_sec) ?                             \
            ((tsp)->tv_nsec cmp (usp)->tv_nsec) :                       \
            ((tsp)->tv_sec cmp (usp)->tv_sec))


/*****************************************************************************
 * PRIVATE DATA TYPES                                                        *
 *****************************************************************************/

typedef enum alSubjectType {
  AL_INVALID = 0,
  AL_FILE_DESCRIPTOR = 1,
  AL_PROCESS,
} alSubjectType;

/*
 * Timer entry.
 */
typedef struct alTimerEntry {
  int id;
  struct timespec targetTime;
  alCallback cb;
  LIST_ENTRY(alTimerEntry) listEntry;
} alTimerEntry;

/*
 * Event tracking.
 */
typedef struct alEventDescriptorEntry {
  alSubjectType subjectType;
  union {
    int fd;
    pid_t pid;
  } subject;
  alCallback cb;
  int active;
  int pendingEventFlags; /* Events pending for this entry */
  int enabledFlags;      /* Event types currently enabled */
  LIST_ENTRY(alEventDescriptorEntry) listEntry;
} alEventDescriptorEntry;

/*
 * Deferred callbacks.
 */
typedef struct alQueuedCallbackEntry {
  alCallback cb;
  void *arg0;
  int arg1;
  SLIST_ENTRY(alQueuedCallbackEntry) listEntry;
} alQueuedCallbackEntry;
  
/*****************************************************************************
 * PRIVATE PROTOTYPES                                                        *
 *****************************************************************************/
static void alEvent__resizeKeventFilters(int newsize);

/*****************************************************************************
 * GLOBALS                                                                   *
 *****************************************************************************/
static LIST_HEAD(, alTimerEntry) alTimerQueue;
static LIST_HEAD(, alEventDescriptorEntry) alEventDescriptors;
static LIST_HEAD(, alEventDescriptorEntry) alDestroyingEventDescriptors;
static SLIST_HEAD(, alQueuedCallbackEntry) alQueuedCallbacks;
static int            alKQFd;
static int            alKeventListSizeChanges;
static int            alKeventListSize;
static int            alNextTimerId;
static struct kevent *alKeventList;

/*****************************************************************************
 * PUBLIC INTERFACES                                                         *
 *****************************************************************************/
 
/*
 * alEvent_init
 *
 * Initializes the event system.  To be called once at application start-up.
 */
int
alEvent_init(void)
{
  alKeventListSize = 0;
  alKeventListSizeChanges = 0;
  alNextTimerId = 0;
  alKeventList = NULL;
  LIST_INIT(&alTimerQueue);
  LIST_INIT(&alEventDescriptors);
  LIST_INIT(&alDestroyingEventDescriptors);
  SLIST_INIT(&alQueuedCallbacks);
  
  alKQFd = kqueue();
  if (alKQFd == -1)
    return -1;
  
  return 0;
}

/*
 * alEvent_shutdown
 *
 * Shuts down the event system, freeing any resources it acquired.
 */
int
alEvent_shutdown(void)
{
  alEventDescriptorEntry *evEntry;
  alQueuedCallbackEntry *cbEntry;
  alTimerEntry *tmEntry;

  while (!LIST_EMPTY(&alTimerQueue)) {
    tmEntry = LIST_FIRST(&alTimerQueue);
    LIST_REMOVE(tmEntry, listEntry);
    free(tmEntry);
  }

  while (!LIST_EMPTY(&alEventDescriptors)) {
    evEntry = LIST_FIRST(&alEventDescriptors);
    LIST_REMOVE(evEntry, listEntry);
    free(evEntry);
  }

  while (!LIST_EMPTY(&alDestroyingEventDescriptors)) {
    evEntry = LIST_FIRST(&alDestroyingEventDescriptors);
    LIST_REMOVE(evEntry, listEntry);
    free(evEntry);
  }

  while (!SLIST_EMPTY(&alQueuedCallbacks)) {
    cbEntry = SLIST_FIRST(&alQueuedCallbacks);
    SLIST_REMOVE_HEAD(&alQueuedCallbacks, listEntry);
    free(cbEntry);
  }

  if (alKeventList != NULL) {
    free(alKeventList);
    alKeventListSize = 0;
  }

  alKeventListSizeChanges = 0;

  return close(alKQFd);
}

/*
 * alEvent_queueCallback
 *
 * Schedule a function to be called at the start of the next event
 * cycle.
 */
int
alEvent_queueCallback(alCallback cb, int flags, void *arg0, int arg1)
{
  alQueuedCallbackEntry *cbEntry;

  if (flags & ALCB_UNIQUE) {
    SLIST_FOREACH(cbEntry, &alQueuedCallbacks, listEntry) {
      if (AL_CALLBACK_EQUAL(&cbEntry->cb, &cb) &&
          cbEntry->arg0 == arg0 && cbEntry->arg1 == arg1)
      {
        return 0;
      }
    }
  }

  cbEntry = alMallocFatal(alQueuedCallbackEntry, 1);
  cbEntry->cb = cb;
  cbEntry->arg0 = arg0;
  cbEntry->arg1 = arg1;

  SLIST_INSERT_HEAD(&alQueuedCallbacks, cbEntry, listEntry);

  return 0;
}

/*
 * alEvent_registerFd
 *
 * Registers a file descriptor with the event system and a callback to call
 * when events occur on that descriptor.
 */
int
alEvent_registerFd(int fd, int flags, alCallback cb, alEventHandle *r)
{
  alEventDescriptorEntry *entry;
  
  /*
   * Do we already have an entry for this descriptor?
   */
  LIST_FOREACH(entry, &alEventDescriptors, listEntry) {
    if (entry->subjectType == AL_FILE_DESCRIPTOR && entry->subject.fd == fd)
      return -1;  /* Already registered */
  }
  
  /*
   * Make a new entry.
   */
  entry = alMallocFatal(alEventDescriptorEntry, 1);
  entry->subjectType = AL_FILE_DESCRIPTOR;
  entry->subject.fd = fd;
  entry->cb = cb;
  entry->active = 1;
  entry->pendingEventFlags = 0;
  entry->enabledFlags = 0;
  LIST_INSERT_HEAD(&alEventDescriptors, entry, listEntry);
  
  /*
   * Tell the kernel to create a read and/or write filter for this
   * descriptor.
   */
  if (alEvent_setFdEvents(entry, flags) != 0)
    goto failed_set_fd_flags;
  
  /*
   * Grow the kevent list to accomodate the kernel returning a descriptor READ
   * and WRITE event when safely possible to reallocate it.
   */
  alKeventListSizeChanges += 2;

  *r = entry;

  return 0;
  
failed_set_fd_flags:
  LIST_REMOVE(entry, listEntry);
  free(entry);
  return -1;
}

/*
 * alEvent_registerProc
 *
 * Registers a process identifier with the event system and a callback to call
 * when events occur with that process.
 */
int
alEvent_registerProc(pid_t pid, int flags, alCallback cb, alEventHandle *r)
{
  alEventDescriptorEntry *entry;

  /*
   * Do we already have an entry for this process?
   */
  LIST_FOREACH(entry, &alEventDescriptors, listEntry) {
    if (entry->subjectType == AL_PROCESS && entry->subject.pid == pid)
      return -1;  /* Already registered */
  }
  
  /*
   * Make a new entry.
   */
  entry = alMallocFatal(alEventDescriptorEntry, 1);
  entry->subjectType = AL_PROCESS;
  entry->subject.pid = pid;
  entry->cb = cb;
  entry->active = 1;
  entry->pendingEventFlags = 0;
  entry->enabledFlags = 0;
  LIST_INSERT_HEAD(&alEventDescriptors, entry, listEntry);
  
  if (alEvent_setProcEvents(entry, flags) != 0)
    goto failed_set_proc_flags;
  
  /*
   * Grow the kevent list to accomodate the kernel returning a process
   * EXIT event when safely possible to reallocate it.
   */
  alKeventListSizeChanges += 1;
  
  *r = entry;

  return 0;

failed_set_proc_flags:
  LIST_REMOVE(entry, listEntry);
  free(entry);
  return -1;
}

/*
 * alEvent_setFdEvents
 *
 * Change the events enabled for a previously registered file descriptor.
 */
int
alEvent_setFdEvents(alEventHandle evp, int newflags)
{
  alEventDescriptorEntry *evh = (alEventDescriptorEntry *) evp;
  struct kevent filters[2];
  int kflags;
  size_t i;
  int fd;

  if (evh->subjectType != AL_FILE_DESCRIPTOR || !evh->active)
    return -1;

  if (evh->enabledFlags == newflags)
    /* No change */
    return 0;

  fd = evh->subject.fd;
  i = 0;

  if ((evh->enabledFlags ^ newflags) & ALFD_READ) {
    kflags = EV_ADD;
    if (newflags & ALFD_READ)
      kflags |= EV_ENABLE;
    else
      kflags |= EV_DISABLE;
    EV_SET(&filters[i], fd, EVFILT_READ, kflags, 0, 0, evh);
    i++;
  }
  if ((evh->enabledFlags ^ newflags) & ALFD_WRITE) {
    kflags = EV_ADD;
    if (newflags & ALFD_WRITE)
      kflags |= EV_ENABLE;
    else
      kflags |= EV_DISABLE;
    EV_SET(&filters[i], fd, EVFILT_WRITE, kflags, 0, 0, evh);
    i++;
  }

  assert(i != 0);

  do {
    if (kevent(alKQFd, filters, i, NULL, 0, NULL) == 0)
      break;
    if (errno != EINTR)
      goto failed_kqueue_add_fd;
  } while (1);

  evh->enabledFlags = newflags;

  return 0;

failed_kqueue_add_fd:
  return -1;
}

/*
 * alEvent_setProcEvents
 *
 * Change the events enabled for a previously registered process id.
 */
int
alEvent_setProcEvents(alEventHandle evp, int newflags)
{
  alEventDescriptorEntry *evh = (alEventDescriptorEntry *) evp;
  struct kevent filter;
  int kflags, fflags;
  size_t i;
  pid_t pid;
  
  if (evh->subjectType != AL_PROCESS || !evh->active)
    return -1;

  if (evh->enabledFlags == newflags)
    /* No change */
    return 0;

  /*
   * We only support one type of event for processes at the moment.
   */
  if ((~newflags & ALPROC_EXIT) != 0)
    return -1;

  pid = evh->subject.pid;
  i = 0;

  if ((evh->enabledFlags ^ newflags) & ALPROC_EXIT) {
    kflags = EV_ADD;
    fflags = NOTE_EXIT;
    if (newflags & ALPROC_EXIT)
      kflags |= EV_ENABLE;
    else
      kflags |= EV_DISABLE;
    
    EV_SET(&filter, pid, EVFILT_PROC, kflags, fflags, 0, evh);
    i++;
  }

  assert(i != 0);

  do {
    if (kevent(alKQFd, &filter, i, NULL, 0, NULL) == 0)
      break;
    if (errno != EINTR)
      goto failed_kqueue_add_proc;
  } while (1);

  evh->enabledFlags = newflags;

  return 0;
  
failed_kqueue_add_proc:
  return -1;
}

/*
 * alEvent_pending
 *
 * Returns true if the event system is still charged with monitoring a
 * file descriptor, has any timers pending, or has any callbacks queued.
 */
int
alEvent_pending(void)
{
  return (!LIST_EMPTY(&alEventDescriptors) || !LIST_EMPTY(&alTimerQueue) ||
          !SLIST_EMPTY(&alQueuedCallbacks));
}

/*
 * alEvent_poll
 *
 * Waits for the next I/O or timer event (whichever comes first).  Should be
 * called repeatedly whenever alEvent_pending() returns true.
 */
void
alEvent_poll(void)
{
  struct timespec waitTime, *pWaitTime = NULL, nowTS;
  struct timeval now;
  alTimerEntry *timerEntry;
  alEventDescriptorEntry *evEntry, *evNext;
  alQueuedCallbackEntry *cbEntry;
  int nevents, i;
  
  /*
   * Issue any queued callbacks.
   */
  while (!SLIST_EMPTY(&alQueuedCallbacks)) {
    cbEntry = SLIST_FIRST(&alQueuedCallbacks);
    SLIST_REMOVE_HEAD(&alQueuedCallbacks, listEntry);
    alEvent_doCallback(cbEntry->cb, cbEntry->arg0, cbEntry->arg1);
    free(cbEntry);
  }
  
  /*
   * Free any event descriptor notifications that are pending deletion.
   */
  while (!LIST_EMPTY(&alDestroyingEventDescriptors)) {
    evEntry = LIST_FIRST(&alDestroyingEventDescriptors);
    LIST_REMOVE(evEntry, listEntry);
    free(evEntry);
  }

  /*
   * If there are any timers pending, compute how soon the earliest one
   * will go off.
   */
  if (!LIST_EMPTY(&alTimerQueue)) {
    /*
     * The timer queue is pre-sorted by expiration time.  Therefore the
     * first entry on the list is guaranteed to be the soonest to expire.
     */
    timerEntry = LIST_FIRST(&alTimerQueue);
    gettimeofday(&now, NULL);
    TIMEVAL_TO_TIMESPEC(&now, &nowTS);
    timespecsub(&timerEntry->targetTime, &nowTS, &waitTime);
    if (waitTime.tv_sec < 0) {
      waitTime.tv_sec = 0;
      waitTime.tv_nsec = 0;
    }
    pWaitTime = &waitTime; 
  } else
    pWaitTime = NULL;
  
  /*
   * Perform any event list size changes that may have been requested
   * since the last poll.
   */
  if (alKeventListSizeChanges != 0) {
    alEvent__resizeKeventFilters(alKeventListSize + alKeventListSizeChanges);
    alKeventListSizeChanges = 0;
  }
  
  /*
   * Wait for events.
   */
  nevents = kevent(alKQFd, NULL, 0, alKeventList, alKeventListSize, pWaitTime);
  if (nevents == -1) {
    if (errno != EINTR)
      err(1, "kevent failed");
    nevents = 0;
  }
  
  /*
   * Note the time at which kevent returned.
   */
  gettimeofday(&now, NULL);
  TIMEVAL_TO_TIMESPEC(&now, &nowTS);
  
  /*
   * Gather up and record all event that have happened.
   */
  for (i = 0; i < nevents; i++) {
    evEntry = (alEventDescriptorEntry *) alKeventList[i].udata;
    if (!evEntry->active)
      continue;
    if (alKeventList[i].filter == EVFILT_READ)
      evEntry->pendingEventFlags |= ALFD_READ;
    if (alKeventList[i].filter == EVFILT_WRITE)
      evEntry->pendingEventFlags |= ALFD_WRITE;
    if (alKeventList[i].filter == EVFILT_PROC) {
      if (alKeventList[i].fflags & NOTE_EXIT)
        evEntry->pendingEventFlags |= ALPROC_EXIT;
    }
  }
  
  /*
   * Issue event callbacks on all triggered event handles.
   */
  for (evEntry = LIST_FIRST(&alEventDescriptors); evEntry != NULL;) {
	/* Get pointer to next entry as this one may come off the list */
	alEventDescriptorEntry *evNext = LIST_NEXT(evEntry, listEntry);
        if (evEntry->pendingEventFlags != 0) {
          alEvent_doCallback(evEntry->cb, evEntry, evEntry->pendingEventFlags);
          evEntry->pendingEventFlags = 0;
        }
        evEntry = evNext;
  }

  /*
   * Issue any timer callbacks that expired.
   */
  while (!LIST_EMPTY(&alTimerQueue)) {
    timerEntry = LIST_FIRST(&alTimerQueue);
    if (timespeccmp(&nowTS, &timerEntry->targetTime, >)) {
      /*
       * This timer has expired and should be triggered.
       */
      LIST_REMOVE(timerEntry, listEntry);
      alEvent_doCallback(timerEntry->cb, NULL, timerEntry->id);
      free(timerEntry);
    } else
      break;
  }
}


int
alEvent_addTimer(int timeMs, int flags, alCallback cb)
{
  struct timeval now, add, tv;
  struct timespec target;
  alTimerEntry *entry, *entry2, *latestEntry = NULL;
  
  /*
   * Allocate a timer entry.
   */
  entry = alMalloc(alTimerEntry, 1);
  if (entry == NULL)
    return -1;
  
  /*
   * Assign it a unique id.
   */
  entry->cb = cb;
  entry->id = ++alNextTimerId;
  if (alNextTimerId < 0)
    alNextTimerId = 0;
  
  /*
   * Compute the target time.
   */
  gettimeofday(&now, NULL);
  add.tv_sec = timeMs / 1000;
  add.tv_usec = (timeMs % 1000) * 1000;
  timeradd(&now, &add, &tv);
  TIMEVAL_TO_TIMESPEC(&tv, &entry->targetTime);
  
  /*
   * Find the first timer that expires after the target
   * time for this entry.
   */
  LIST_FOREACH(entry2, &alTimerQueue, listEntry) {
    if (timespeccmp(&entry2->targetTime, &entry->targetTime, >))
      break;
    latestEntry = entry2;
  }
  if (latestEntry == NULL)
    LIST_INSERT_HEAD(&alTimerQueue, entry, listEntry);
  else
    LIST_INSERT_AFTER(latestEntry, entry, listEntry);
  
  return entry->id;
}

int
alEvent_cancelTimer(int id)
{
  alTimerEntry *entry;
  
  LIST_FOREACH(entry, &alTimerQueue, listEntry) {
    if (entry->id == id) {
      LIST_REMOVE(entry, listEntry);
      free(entry);
      return 0;
    }
  }
  
  /* Entry not found */
  return -1;
}

int
alEvent_deregister(alEventHandle handle)
{
  struct kevent ev[2];
  int inspect_failure;
  size_t i, sizechanges;
  
  alEventDescriptorEntry *entry = (alEventDescriptorEntry *) handle;

  if (entry->subjectType == AL_INVALID)
    return -1;

  i = 0;

  if (entry->active) {
    LIST_REMOVE(entry, listEntry);
    LIST_INSERT_HEAD(&alDestroyingEventDescriptors, entry, listEntry);
    switch (entry->subjectType) {
    case AL_INVALID:
      assert(0); /* Shouldn't happen */
    case AL_FILE_DESCRIPTOR:
      if (entry->enabledFlags & ALFD_READ)
        EV_SET(&ev[i++], entry->subject.fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
      if (entry->enabledFlags & ALFD_WRITE)
        EV_SET(&ev[i++], entry->subject.fd, EVFILT_WRITE, EV_DELETE, 0, 0,NULL);
      inspect_failure = 1;
      sizechanges = 2;
      break;
    case AL_PROCESS:
      if (entry->enabledFlags & ALPROC_EXIT)
        EV_SET(&ev[i++], entry->subject.pid, EVFILT_PROC, EV_DELETE, 0, 0,NULL);
      inspect_failure = 0; /* Process may have exited */
      sizechanges = 1;
      break;
    }

    if (i > 0)
      if (kevent(alKQFd, ev, i, NULL, 0, NULL) != 0 && inspect_failure)
        err(1, "kevent remove failed");

    /* Request that the kevent list be shrunk when safely possible */
    alKeventListSizeChanges -= sizechanges;
    entry->active = 0;
    return 0;
  } else {
    return -1;
  }
}

/*****************************************************************************
 * PRIVATE INTERFACES                                                        *
 *****************************************************************************/

static void
alEvent__resizeKeventFilters(int newsize)
{
  int i, j;
  
  if (newsize < 64)
    newsize = 64;
  
  if (newsize == alKeventListSize)
    return;
  
  /*
   * Bring newsize to nearest power of two.
   * Calculate log2(newsize).
   */
  for (i = 0, j = newsize; j > 0; i++, j >>= 1)
    ;
  if (newsize > (1 << i))
    newsize = (1 << (i+1));

  if (newsize != alKeventListSize) {
    alKeventList = realloc(alKeventList, newsize * sizeof(struct kevent));
    alKeventListSize = newsize;
  }
}
