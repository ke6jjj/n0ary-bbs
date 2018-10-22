#include <sys/select.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "c_cmmn.h"
#include "bbslib.h"
#include "bbscommon.h"
#include "function.h"
#include "user.h"
#include "tools.h"
#include "vars.h"

/*
 * I/O STACK
 *
 * This picture represents how input and output flow through the BBS using
 * the various calls.
 *
 *  GETS()        PRINTF()       PRINT()
 *    |              |               |
 *    v              v               v
 * user_gets()   user_printf()   user_puts()           user_write()
 *    |              |               |                      |
 *    |              |   fd_putln()  |                      |
 *    |              |       |       |                      |
 *    |              |       v       |                      |
 *    |              +<- fd_printf() |                      |
 *    v              v               v                      | 
 * fd_gets()     fd_vprintf() -> fd_puts() -> UpperCase     |
 *    |                              |           |          |
 *    |                              v           v          v
 *    +------------------------>  LOGGING      Xlate -> fd_write()
 *    |                           MONITORING   CR/LF        |
 *    v                           log_user()                v
 *  Monitor check                                       TNC safeing
 *  Prompt change <--bbsd/chat                           (optional)
 *    |                                                     |
 *    v                                                     v
 *  socket_read_line()                              socket_raw_write_n()
 */

static void write_socket_translate_nl(const char *buf, size_t len);
static void log_user(char *str);
static const char *mem2chr(const char *src, const char *pat, size_t len);
static void fd_write_translate_nl(int fd, const char *buf, size_t len);
static int fd_tncd_safe_write(int fd, const char *buf, size_t len);

/* Read a line from the default input source */
char *
user_gets(char *p, int cnt)
{
	int fd = (sock == ERROR) ? STDIN_FILENO : sock;

	return fd_gets(fd, p, cnt);
}

/* Read a line from a specific file descriptor */
char *
fd_gets(int fd, char *p, int cnt)
{
	char *str = p;
	fd_set ready;
	int done = FALSE;
	int idletime = 0;

	if(socket_read_pending(fd) == TRUE) {
		if(socket_read_line(fd, str, cnt, 30) == sockERROR) {
			error_log("fd_gets.read(): %s", sys_errlist[errno]);
			return NULL;
		}

		if(monitor_fd != ERROR)
			socket_raw_write(monitor_fd, p);

		if(ImLogging)
			log_user(p);
		return p;
	}

	/* first check for input off of the bbsd port */

	while(!done) {
		struct timeval t;
		int result;
		int fdlimit = fd;

		FD_ZERO(&ready);
		FD_SET(fd, &ready);
		if(monitor_fd != ERROR) {
			FD_SET(monitor_fd, &ready);
			if(monitor_fd > fdlimit)
				fdlimit = monitor_fd;
		} else {
			if(monitor_sock != ERROR) {
				FD_SET(monitor_sock, &ready);
				if(monitor_sock > fdlimit)
					fdlimit = monitor_sock;
			}
		}
		FD_SET(bbsd_sock, &ready);
		if(bbsd_sock > fdlimit)
			fdlimit = bbsd_sock;

		fdlimit++;

		if(inactivity_time) {
			t.tv_sec = PING_INTERVAL;
			t.tv_usec = 0;
			result = select(fdlimit, (void*)&ready, NULL, NULL, &t);
		} else
			result = select(fdlimit, (void*)&ready, NULL, NULL, NULL);

		switch(result) {
		case 0:
			idletime += PING_INTERVAL;
			if(idletime > inactivity_time) {
				PRINTF("Timeout occured, link was idle for %d minutes.\n",
					inactivity_time / 60);
				return NULL;
			}
#if 0
			bbsd_ping();
#endif
			continue;

		case ERROR:
			PRINTF("select: %s\n", sys_errlist[errno]);
			continue;
		}

		if(monitor_sock != ERROR)
			if(FD_ISSET(monitor_sock, &ready)) {
				char buf[80];
				if((monitor_fd = socket_accept(monitor_sock)) < 0)
					monitor_fd = ERROR;
				sprintf(buf, "Connected to %s\n", usercall);
				socket_raw_write(monitor_fd, buf);
				continue;
			}

		if(monitor_fd != ERROR)
			if(FD_ISSET(monitor_fd, &ready)) {
				if(monitor_service() == ERROR) {
					socket_close(monitor_fd);
					monitor_fd = ERROR;
				}
				continue;
			}

		if(FD_ISSET(bbsd_sock, &ready)) {
			char *c = bbsd_read();
			if(c == NULL) {
	PRINT("Lost connection with bbsd, please reconnect in a few minutes.\n");
				return NULL;
			}
			switch(*c++) {
			case 'P':
				strcpy(prompt_string, c);
				break;
			case 'I':
				PRINTF("%s\n", c);
				break;
			}
		}

		if(FD_ISSET(fd, &ready)) {
			if(socket_read_line(fd, str, cnt, 30) == sockERROR) {
				error_log("fd_gets.read(): %s", sys_errlist[errno]);
				return NULL;
			}
			done = TRUE;
		}
	}

	if(monitor_fd != ERROR)
		socket_raw_write(monitor_fd, p);

	if(ImLogging)
		log_user(p);

	return p;
}

/* Print a formatted string to the default output */
void
user_printf(char *fmt, ...)
{
	int fd = (sock == ERROR) ? STDOUT_FILENO : sock;
	va_list ap;

	va_start(ap, fmt);
	fd_vprintf(fd, fmt, ap);
	va_end(ap);
}

/* Print a formatted string to a specific file descriptor */
void
fd_printf(int fd, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fd_vprintf(fd, fmt, ap);
	va_end(ap);
}

/* Print a formatted string to a specific file descriptor (va_list) */
void
fd_vprintf(int fd, const char *fmt, va_list ap)
{
	char buf[4096];
	int err;

	vsnprintf(buf, sizeof(buf)-1, fmt, ap);

	buf[sizeof(buf)-1] = '\0';

	fd_puts(fd, buf);
}

/* Print a nul-terminated string to the default output */
void
user_puts(char *buf)
{
	int fd = (sock == ERROR) ? STDOUT_FILENO : sock;

	fd_puts(fd, buf);
}

/* Print a nul-terminated string to a specific file descriptor */
void
fd_puts(int fd, char *buf)
{
	size_t len;

	if(Cnvrt2Uppercase)
		uppercase(buf);

	if(ImLogging)
		log_user(buf);

	len = strlen(buf);

	if(monitor_fd != ERROR && monitor_connected == FALSE)
		socket_raw_write_n(monitor_fd, buf, len);

	if (DoCRLFEndings)
		fd_write_translate_nl(fd, buf, len);
	else
		fd_write(fd, buf, len);
}

void
fd_putln(int fd, char *buf)
{
	fd_printf(fd, "%s\n", buf);
}

/* Write arbitrary (possibly binary) character data to the default output.
 * No logging nor newline mapping will be made.
 * Output will be filtered for TNC commands if applicable, however.
 */
void
user_write(const char *buf, size_t len)
{
	int fd = (sock == ERROR) ? STDOUT_FILENO : sock;

	fd_write(fd, buf, len);
}

/* Write arbitrary (possibly binary) character data to a specific file
 * descriptor.
 * No logging nor newline mapping will be made.
 * Output will be filtered for TNC commands if applicable, however.
 */
void
fd_write(int fd, const char *buf, size_t len)
{
	int err;

	if (escape_tnc_commands)
		err = fd_tncd_safe_write(fd, buf, len);
	else
		err = socket_raw_write_n(fd, buf, len);

	if (err == ERROR)
		error_log("write_socket.write(): %s", sys_errlist[errno]);
}

static void
fd_write_translate_nl(int fd, const char *buf, size_t len)
{
	const char *nl;

	while (len > 0 && (nl = memchr(buf, '\n', len)) != NULL) {
		if (nl != buf)
			fd_write(fd, buf, nl - buf);
		fd_write(fd, "\r\n", 2);
		len -= nl - buf + 1;
		buf = nl + 1;
	}
	if (len > 0)
		fd_write(fd, buf, len);
}

static void
log_user(char *str)
{
	if(logfile == NULL) {
		char fn[80];
		time_t t = Time(NULL);
		struct tm *dt = localtime(&t);

		sprintf(fn, "%s/%s", Bbs_Log_Path, usercall);
		if((logfile = fopen(fn, "a+")) == NULL)
			return;
		
		fprintf(logfile, "\n===========================\n");
		fprintf(logfile, "======[%02d/%02d @ %02d:%02d]======\n",
			dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min);
		fprintf(logfile, "===========================\n");
	}

	if(index(str, '\n'))
		fprintf(logfile, "%s", str);
	else
		fprintf(logfile, "%s\n", str);
	fflush(logfile);
}

/*
 * Write a buffer to the user output socket in such a way that no in-band
 * TNC commands will appear in the stream (this is a requirement when talking
 * to an AX.25 connection that is mediated by tncd).
 */
static int
fd_tncd_safe_write(int sock, const char *buf, size_t len)
{
	/* I hope there's only one socket in use: */
	static int tnc_command_state = 0;
	static int tnc_command_fd = -1;
	const char *unsafe;
	ssize_t err;

	if (len < 1)
		return 0;

	if (tnc_command_fd == -1)
		tnc_command_fd = sock;

	/* See if this is in use on multiple sockets! */
	assert(sock == tnc_command_fd);

	if (tnc_command_state == 1 && buf[0] == '~') {
		err = socket_raw_write_n(sock, "~", 1);
		if (err == ERROR)
			return ERROR;
		tnc_command_state = 0;
	}

	while ((unsafe = mem2chr(buf, "\n~", len)) != NULL) {
		size_t subsz = unsafe - buf + 2;
		err = socket_raw_write_n(sock, buf, subsz);
		if (err == ERROR)
			return ERROR;
		err = socket_raw_write_n(sock, "~", 1);
		if (err == ERROR)
			return ERROR;
		buf += subsz;
		len -= subsz;
	}

	if (len) {
		err = socket_raw_write_n(sock, buf, len);
		if (err == ERROR)
			return ERROR;
		tnc_command_state = buf[len-1] == '\n';
	}

	return 0;
}

static const char *
mem2chr(const char *src, const char *pat, size_t len)
{
	const char *f;

	do {
		f = memchr(src, pat[0], len);
		if (f == NULL)
			return NULL;
		len -= f - src + 1;
		if (len == 0)
			return NULL;
		if (f[1] == pat[1])
			return f;
		src = f + 1;
		len--;
	} while (len > 0);

	return NULL;
}
