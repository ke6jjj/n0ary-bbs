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
#ifndef JSC_ALIB_H
#define JSC_ALIB_H

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
 * Callback queueing flags.
 */
enum alCbEnqueue_flags {
  ALCB_UNIQUE = 1,  /* Only enqueue callback if it is not already queued */
};

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
 * alEvent_queueCallback
 *
 * Schedule a function to be called at the start of the next event
 * cycle.
 */
int alEvent_queueCallback(alCallback cb, int flags, void *arg0, int arg1);

/*
 * alEvent_registerFd
 *
 * Registers a file descriptor with the event system and a callback to call
 * when events occur on that descriptor.
 */
int alEvent_registerFd(int fd, int flags, alCallback cb,
	alEventHandle *rhandle);

/*
 * alEvent_setFdEvents
 *
 * Change the events enabled for a previously registered file descriptor.
 */
int alEvent_setFdEvents(alEventHandle evh, int newflags);

/*
 * alEvent_registerProc
 *
 * Registers a process identifier with the event system and a callback to
 * call when events occur with that process.
 */
int alEvent_registerProc(pid_t pid, int flags, alCallback cb,
	alEventHandle *rhandle);

/*
 * alEvent_setProcEvents
 *
 * Change the events enabled for a previously registered process id.
 */
int alEvent_setProcEvents(alEventHandle evh, int newflags);

/*
 * alEvent_deregister
 *
 * Unregisters the previously registered event descriptor described by
 * 'handle' from the notification system.
 */
int alEvent_deregister(alEventHandle handle);

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

/*
 * alEvent_addTimer
 *
 * Schedule a callback to be issued after a certain time period has elapsed.
 */
int alEvent_addTimer(int timeMs, int flags, alCallback cb);

/*
 * alEvent_cancelTimer
 *
 * Cancel a previously started timer. (It is safe to cancel a timer that
 * has already expired provided that it is done within 2 billion (2x10^9)
 * interceeding timer registrations).
 */
int alEvent_cancelTimer(int id);

#endif
