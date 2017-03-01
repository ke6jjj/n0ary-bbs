#include <stdio.h>
#include <fcntl.h>
#ifdef SUNOS
#include <termio.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "c_cmmn.h"
#include "tools.h"

#if 0
	#include <fcntl.h>
	#include <sys/termios.h>

	sturct tty tty;

	tty.mode = O_RDWR;
	tty.device = SOLA_DEVICE; 
	tty.oflag = OPOST|ONLCR; 
	tty.lflag = 0;
	tty.cflag = B9600|CS8|CSTOPB|CREAD;
	tty.fndelay = TRUE;
	fd = open_tty(&tty);

		returns ERROR if there is a problem.
#endif

int
open_tty(struct tty *t)
{
	struct termios tt;
	int fd;

	if((fd = open(t->device, t->mode)) == ERROR)
		return fd;

#ifdef TCGETS
	if(ioctl(fd, TCGETS, &tt)) {
		close(fd);
		return ERROR;
	}
#else
 	if(tcgetattr(fd, &tt)) {
		close(fd);
		return ERROR;
	}
#endif

	tt.c_oflag = t->oflag;
	tt.c_lflag = t->lflag;
	tt.c_cflag = t->cflag;
	tt.c_iflag = t->iflag;

#ifdef TCSETS
	if(ioctl(fd, TCSETS, &tt)) {
		close(fd);
		return ERROR;
	}
#else
 	if(tcsetattr(fd, 0, &tt)) {
		close(fd);
		return ERROR;
	}
#endif

#ifdef F_SETOWN			/* Is this really needed? -- rwp */
	if(fcntl(fd, F_SETOWN, getpid()) < 0) {
		close(fd);
		return ERROR;
	}
#endif

	if(t->fndelay == TRUE)
		if(fcntl(fd, F_SETFL, O_NDELAY) < 0) {
			close(fd);
			return ERROR;
		}

	return fd;
}

int
close_tty(int fd)
{
	if(fd != ERROR)
		close(fd);
	return ERROR;
}

