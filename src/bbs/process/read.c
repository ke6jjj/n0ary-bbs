#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

#include "c_cmmn.h"
#include "config.h"

int
read_line(int fd, char *buf, int cnt)
{
	fd_set ready;
	struct timeval timeout;

	FD_ZERO(&ready);
	FD_SET(fd, &ready);
	timeout.tv_sec = 120;
	timeout.tv_usec = 0;

	if(select(64, &ready, 0, 0, &timeout) == 0) {
		return TRUE;
	}

	while(cnt--) {
		if(read(fd, buf, 1) == ERROR)
			return ERROR;
		if(*buf == '\n')
			break;
		buf++;
	}
	*buf = 0;
	return OK;	
}

int
wait_for_line(int fd, char *match)
{
	if((re_comp(match)) != NULL)
		return ERROR;

	while(TRUE) {
		char buf[1024];
		switch(read_line(fd, buf, 1024)) {
		case TRUE:
			return TRUE;
		case ERROR:
			return ERROR;
		case OK:
			if(re_exec(buf))
				return OK;
		}
	}
}
