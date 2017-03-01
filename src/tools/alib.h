#define alEvent_doCallback(cb, arg0, arg1) ((cb).fn((cb).obj, arg0, arg1))
#define AL_CALLBACK(cb, o, f) { (cb)->obj = o; (cb)->fn = f; }
#define alMalloc(type, size) ((type *)malloc(sizeof(type)*size))
#define alMallocFatal(type, size) ((type *)malloc(sizeof(type)*size))

/*
 * alCallbackFn
 *
 * Prototype for all event callback functions.
 */
typedef void (alCallbackFn)(void *obj, void *arg0, int arg1);

/*
 * alCallback
 */
typedef struct {
  void *obj;         /* The object being called */
  alCallbackFn *fn;  /* Callback function on that object */
} alCallback;

/*
 * Event notification handle.
 */
typedef void *alEventHandle;
   
/*
 * File descriptor notification flags.
 */
enum alFdEvent_flags {
  ALFD_READ = 1,
  ALFD_WRITE = (1 << 1)
};

/*
 * Process notification flags.
 */
enum alProcEvent_flags {
  ALPROC_EXIT = 1,
};
  
/*
 * alEvent_init
 *
 * Initializes the event system.  To be called once at application start-up.
 */
int alEvent_init(void);

/*
 * alEvent_shutdown
 *
 * Shuts down the event system and frees any allocated resources for it.
 */
int alEvent_shutdown(void);

/*
 * alEvent_addFdCallback
 *
 * Registers a file descriptor with the event system and a callback to call
 * when events occur on that descriptor.
 */
int alEvent_addFdCallback(int fd, int flags, alCallback cb,
	alEventHandle *rhandle);

/*
 * alEvent_addProcCallback
 *
 * Registers a process identifier with the event system and a callback to
 * call when events occur with that process.
 */
int alEvent_addProcCallback(pid_t pid, int flags, alCallback cb,
	alEventHandle *rhandle);

/*
 * alEvent_removeEventCallback
 *
 * Unregisters the previously registered event descriptor described by
 * 'handle' from the notification system.
 */
int alEvent_removeEventCallback(alEventHandle handle);

/*
 * alEvent_pending
 *
 * Returns true if the event system is still charged with monitoring a
 * file descriptor, has any timers pending, or has any callbacks queued.
 */
int alEvent_pending(void);

/*
 * alEvent_poll
 *
 * Waits for the next I/O or timer event (whichever comes first).  Should be
 * called repeatedly whenever alEvent_pending() returns true.
 */
void alEvent_poll(void);

int alEvent_addTimer(int timeMs, int flags, alCallback cb);
int alEvent_cancelTimer(int id);
