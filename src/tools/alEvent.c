#include <sys/time.h>
#include <sys/queue.h>
#include <sys/event.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>

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
 * File descriptor tracking.
 */
typedef struct alFileDescriptorEntry {
  int fd;
  alCallback cb;
  int active;
  LIST_ENTRY(alFileDescriptorEntry) listEntry;
} alFileDescriptorEntry;

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
static LIST_HEAD(, alFileDescriptorEntry) alFileDescriptors;
static LIST_HEAD(, alFileDescriptorEntry) alDestroyingFileDescriptors;
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
  LIST_INIT(&alFileDescriptors);
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
  alFileDescriptorEntry *fdEntry;
  alQueuedCallbackEntry *cbEntry;
  alTimerEntry *tmEntry;

  while (!LIST_EMPTY(&alTimerQueue)) {
    tmEntry = LIST_FIRST(&alTimerQueue);
    LIST_REMOVE(tmEntry, listEntry);
    free(tmEntry);
  }

  while (!LIST_EMPTY(&alFileDescriptors)) {
    fdEntry = LIST_FIRST(&alFileDescriptors);
    LIST_REMOVE(fdEntry, listEntry);
    free(fdEntry);
  }

  while (!LIST_EMPTY(&alDestroyingFileDescriptors)) {
    fdEntry = LIST_FIRST(&alDestroyingFileDescriptors);
    LIST_REMOVE(fdEntry, listEntry);
    free(fdEntry);
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
 * alEvent_addFileDescriptorNotification
 *
 * Registers a file descriptor with the event system and a callback to call
 * when events occur on that descriptor.
 */
alFileHandle
alEvent_addFileDescriptorNotification(int fd, int flags, alCallback cb)
{
  alFileDescriptorEntry *entry;
  struct kevent filters[2];
  int kflags;
  
  /*
   * Do we already have an entry for this descriptor?
   */
  LIST_FOREACH(entry, &alFileDescriptors, listEntry) {
    if (entry->fd == fd)
      return NULL;  /* Already registered */
  }
  
  /*
   * Make a new entry.
   */
  entry = alMallocFatal(alFileDescriptorEntry, 1);
  entry->fd = fd;
  entry->cb = cb;
  entry->active = 1;
  LIST_INSERT_HEAD(&alFileDescriptors, entry, listEntry);
  
  /*
   * Tell the kernel to create a read and/or write filter for this
   * descriptor.
   */
  kflags = EV_ADD;
  if (!(flags & ALEVENT_READ))
    kflags |= EV_DISABLE;
  EV_SET(&filters[0], fd, EVFILT_READ, kflags, 0, 0, entry);
  kflags = EV_ADD;
  if (!(flags & ALEVENT_WRITE))
    kflags |= EV_DISABLE;
  EV_SET(&filters[1], fd, EVFILT_WRITE, kflags, 0, 0, entry);

  if (kevent(alKQFd, filters, 2, NULL, 0, NULL) != 0)
    goto failed_kqueue_add;
  
  /*
   * Grow the kevent list to accomodate the kernel returning a descriptor READ
   * and WRITE event when safely possible to reallocate it.
   */
  alKeventListSizeChanges += 2;
  
  return entry;
  
failed_kqueue_add:
  LIST_REMOVE(entry, listEntry);
  free(entry);
  return NULL;
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
  return (!LIST_EMPTY(&alFileDescriptors) || !LIST_EMPTY(&alTimerQueue) ||
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
  alFileDescriptorEntry *fdEntry;
  alQueuedCallbackEntry *cbEntry;
  int nevents, i, flags;
  
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
   * Free any file descriptor notifications that are pending deletion.
   */
  while (!LIST_EMPTY(&alDestroyingFileDescriptors)) {
    fdEntry = LIST_FIRST(&alDestroyingFileDescriptors);
    LIST_REMOVE(fdEntry, listEntry);
    free(fdEntry);
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
  if (nevents == -1)
    err(1, "kevent failed");
  
  /*
   * Note the time at which kevent returned.
   */
  gettimeofday(&now, NULL);
  TIMEVAL_TO_TIMESPEC(&now, &nowTS);
  
  /*
   * Perform all file descriptor notifications that have been triggered.
   */
  for (i = 0; i < nevents; i++) {
    fdEntry = (alFileDescriptorEntry *) alKeventList[i].udata;
    if (!fdEntry->active)
      continue;
    flags = 0;
    if (alKeventList[i].filter == EVFILT_READ)
      flags |= ALEVENT_READ;
    if (alKeventList[i].filter == EVFILT_WRITE)
      flags |= ALEVENT_WRITE;
    alEvent_doCallback(fdEntry->cb, NULL, flags);
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
alEvent_removeFdCallback(alFileHandle handle)
{
  struct kevent ev[2];
  
  alFileDescriptorEntry *entry = (alFileDescriptorEntry *) handle;
  if (entry->active) {
    LIST_REMOVE(entry, listEntry);
    LIST_INSERT_HEAD(&alDestroyingFileDescriptors, entry, listEntry);
    EV_SET(&ev[0], entry->fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&ev[1], entry->fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    if (kevent(alKQFd, ev, 2, NULL, 0, NULL) != 0)
      err(1, "kevent remove failed");
    /* Request that the kevent list be shrunk when safely possible */
    alKeventListSizeChanges -= 2;
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
