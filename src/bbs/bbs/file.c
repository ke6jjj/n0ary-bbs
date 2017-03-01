#include <unistd.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "c_cmmn.h"
#include "config.h"
#include "function.h"
#include "file.h"
#include "bbslib.h"
#include "vars.h"

int
Open(char *filename, int mode)
{
	int fd = open(filename, mode);

#ifdef F_LOCK
	if(fd >= 0)
		lockf(fd, F_LOCK, 0);
#else
	if(fd >= 0)
		flock(fd, LOCK_EX);
#endif

	return fd;
}

void
Close(int filedes)
{
#ifdef F_ULOCK
	lockf(filedes, F_ULOCK, 0);
#else
	flock(filedes, LOCK_UN);
#endif
	close(filedes);
}

int
file_last_altered(char *filename)
{
	struct stat sbuf;
	if(stat(filename, &sbuf))
		return ERROR;
	return (int)sbuf.st_mtime;
}

