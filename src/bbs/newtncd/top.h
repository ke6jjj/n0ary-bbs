#ifndef TOP_H
#define TOP_H

#ifdef HOST_BSD
#define HAVE_STDINT
#define USE_SYSTEM_MALLOC
#define HAVE_STRDUP
#define HAVE_UNIX_TIMEOFDAY
#define HAVE_FUNOPEN
#define NO_STD_DUPLICATION
#define USE_SYSTEM_SPRINTF
#define UNIX
#define MODERN_UNIX
#endif

#endif
