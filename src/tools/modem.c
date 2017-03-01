#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "tools.h"

#define sSTART		1
#define sECHO		2
#define sVERB		3
#define sINIT       4
#define sDIAL		5
#define sRECOVER    6
#define sDONE		7

static int debug_fd = ERROR;
static int baudrate = B38400;

void
modem_baudrate(int baud)
{
	switch(baud) {
	case 1200:	baudrate = B1200; break;
	case 2400:	baudrate = B2400; break;
	case 9600:	baudrate = B9600; break;
	case 19200:	baudrate = B19200; break;
	case 38400:	baudrate = B38400; break;
	default:
		printf("Unknown baud rate %d\n", baud);
		baudrate = B38400;
	}
}

void
modem_debug(int fd)
{
	debug_fd = fd;
}

static void
modem_debug_write(char *s)
{
	if(debug_fd != ERROR) {
		char dbug[256];
		sprintf(dbug, "%s\n", s);
		write(debug_fd, dbug, strlen(dbug));
	}
}

int
modem_write(int fd, char *s)
{
	char buf[1024];

	sprintf(buf, "%s\n", s);
	write(fd, buf, strlen(buf));
	if(debug_fd != ERROR) {
		char dbug[80];
		sprintf(dbug, "S: %s", s);
		modem_debug_write(dbug);
	}

	return OK;
}

char *
modem_read(int fd, int timeout)
{
	fd_set ready;
	struct timeval to;
	int done = FALSE;
	static char inbuf[1024];
	char *p = inbuf;
	int cnt = 0;

	*p = 0;

	while(!done) {
		FD_ZERO(&ready);
		FD_SET(fd, &ready);
		to.tv_sec = timeout;
		to.tv_usec = 0;

		switch(select(fd+1, &ready, 0, 0, &to)) {
		case ERROR:
		case 0:
			return NULL;
		default:
			if(!FD_ISSET(fd, &ready))
				return NULL;
			while(read(fd, p, 1) == 1) {
				cnt++;
				if(*p == '\r') {
					done = TRUE;
					*p = 0;
					break;
				}

				if(*p == '\n') {
					*p = 0;
					continue;
				}

				p++;
				*p = 0;
			}
		}
	}
	if(debug_fd != ERROR) {
		char dbug[80];
		sprintf(dbug, "R: %s", inbuf);
		modem_debug_write(dbug);
	}

	if(cnt)
		return inbuf;
	return NULL;
}

int
modem_close(int fd)
{
	if(fd != ERROR) {
		modem_debug_write("X: Close modem");
		close(fd);
	}
	return ERROR;
}

int
modem_open(char *device)
{
	struct termios tt;

	int fd = open(device, O_RDWR|O_NOCTTY);

	if(fd < 0)
		return error_log("Failure opening modem at %s", device);

	modem_debug_write("X: Open modem");

#ifdef HAVE_TERMIOS
 	if(tcgetattr(fd,&tt))
 		return error_log("tcgetattr(fd,&tt): %s", sys_errlist[errno]);
#else
#ifdef TCGETS
	if(ioctl(fd, TCGETS, &tt))
		return error_log("ioctl(fd,TCGETS): %s", sys_errlist[errno]);
#else
#error "Need termios"
#endif
#endif

	tt.c_oflag = OPOST|ONLCR;
	tt.c_lflag = 0;
	tt.c_cflag = baudrate|CS8|CSTOPB|CREAD;
	tt.c_iflag = 0;

#ifdef HAVE_TERMIOS
 	if(tcsetattr(fd, 0, &tt))
 		return error_log("tcsets(fd,&tt): %s", sys_errlist[errno]);
#else
#ifdef TCGETS
	if(ioctl(fd, TCSETS, &tt))
		return error_log("ioctl(fd,TCSETS): %s", sys_errlist[errno]);
#else
#error "Need termios"
#endif
#endif

	if(fcntl(fd, F_SETOWN, getpid()) < 0)
		return error_log("fcntl(fd,F_SETOWN): %s", sys_errlist[errno]);

	if(fcntl(fd, F_SETFL, FNDELAY) < 0)
			error_log("fcntl(fd,F_SETFL,FNDELAY): %s", sys_errlist[errno]);

	return fd;
}

int
modem_dial(char *device, char *phone_number, char *init)
{
	int fd;
	char *p;

	if((fd = modem_blind_dial(device, phone_number, init)) == ERROR)
		return ERROR;

	if((p = modem_read(fd, 60)) == NULL)
		return error_log("timeout waiting for response to ATDT");

	if(strcmp(p, "1"))
		return error_log("expected 1 but got %s instead", p);
	return fd;
}

int
modem_blind_dial(char *device, char *phone_number, char *init)
{
	int fd = modem_open(device);
	int recover = 1;
	int state = sSTART;
	char *p, buf[80];

	if(fd == ERROR) {
		close(fd);
		error_report(modem_debug_write, TRUE);
		return ERROR;
	}

	while(state != sDONE) {
		switch(state) {
		case sRECOVER:
			modem_debug_write("X: RECOVER");
			if(recover-- == 0)
				return error_log("Too many attempt to recover");
		case sSTART:
			modem_debug_write("X: START");
			modem_flush(fd);
			modem_write(fd, "AT");
			if((p = modem_read(fd, 10)) == NULL)
				return error_log("timeout waiting for response to AT");

			if(!strncmp(p, "AT", 2))
				state = sECHO;
			else if(!strncmp(p, "OK", 2))
				state = sVERB;
			else if(!strncmp(p, "0", 1))
				state = (init) ? sINIT:sDIAL;
			else if(!strncmp(p, "3", 1))
				state = sRECOVER;
			else
				return error_log("expected AT/OK/0 but got %s instead", p);
			break;

		case sECHO:
			modem_debug_write("X: ECHO");

			modem_flush(fd);
			modem_write(fd, "ATV0E0X0");
			if((p = modem_read(fd, 10)) == NULL)
				return error_log("timeout waiting for echo of ATV0E0X0");
			if((p = modem_read(fd, 10)) == NULL)
				return error_log("timeout waiting for response to ATV0E0X0");

			if(strncmp(p, "0", 1))
				return error_log("expected 0 but got %s instead", p);
			state = (init) ? sINIT:sDIAL;
			break;
			
		case sVERB:
			modem_debug_write("X: VERB");

			modem_flush(fd);
			modem_write(fd, "ATV0X0");
			if((p = modem_read(fd, 10)) == NULL)
				return error_log("timeout waiting for response to ATV0X0");

			if(strncmp(p, "0", 1))
				return error_log("expected 0 but got %s instead", p);

			state = (init) ? sINIT:sDIAL;
			break;

		case sINIT:
			modem_debug_write("X: INIT");
			modem_write(fd, init);
			if((p = modem_read(fd, 10)) == NULL)
				return error_log("timeout waiting for response to INIT %s", 
								 init);

			if(strncmp(p, "0", 1))
				return error_log("expected 0 but got %s instead", p);
			state = sDIAL;
			break;

		case sDIAL:
			modem_debug_write("X: DIAL");

			sprintf(buf, "ATDT%s", phone_number);
			modem_write(fd, buf);
			state = sDONE;
			break;
		}
	}
	return fd;
}

void
modem_flush(int fd)
{
	modem_debug_write("X: FLUSH");
/*EMPTY*/
	while(modem_read(fd, 1) != NULL);
	modem_debug_write("X: FLUSH DONE");
}
