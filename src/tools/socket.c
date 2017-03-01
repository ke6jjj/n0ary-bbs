#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "c_cmmn.h"
#include "tools.h"

extern char *sys_errlist[];

static int socket_watch = ERROR;

void
socket_watcher(int sock_fd)
{
	socket_watch = sock_fd;
}

static void
socket_print(char c, char *s)
{
	printf("%c(%d):", c, strlen(s));
	while(*s) {
		if(isprint(*s))
			putchar(*s);
		else
			printf("[%02x]", (unsigned char) *s);
		s++;
	}
	putchar('\n');
	fflush(stdout);
}

static struct fd_list {
	struct fd_list *next;
	int fd;
	char save[256];
} *fdlist = NULL;

static struct fd_list *
find_fd(int fd)
{
	struct fd_list *fdl = fdlist;
	while(fdl) {
		if(fdl->fd == fd)
			return fdl;
		NEXT(fdl);
	}
	return NULL;
}

static struct fd_list *
link_fd(int fd)
{
	struct fd_list *fdl = find_fd(ERROR);

	if(fdl == NULL) {
		fdl = malloc_struct(fd_list);
		fdl->next = fdlist;
		fdlist = fdl;
	}
	fdl->fd = fd;
	return fdl;
}

static int
unlink_fd(int fd)
{
	struct fd_list *fdl = find_fd(fd);

	if(fdl != NULL) {
		fdl->fd = ERROR;
		return OK;
	}
	return ERROR;
}


int
socket_listen(int *port)
{
	struct sockaddr_in server;
	int length = sizeof(server);
	int sock = ERROR;
	int linger = 0;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(*port);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return error_log("socket_listen.socket: %s", sys_errlist[errno]);

	if(bind(sock, (struct sockaddr *)&server, length) < 0)
		return  error_log("socket_listen.bind: %s", sys_errlist[errno]);

	if(getsockname(sock, (struct sockaddr *)&server, &length) < 0)
		return error_log("socket_listen.getsockname: %s", sys_errlist[errno]);

	setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
	listen(sock, 5);

#if 0
	*port = server.sin_port;
#endif
	*port = ntohs(server.sin_port);
	return sock;
}

int
socket_accept(int sock)
{
	int fd;
	int linger = 0;

	if((fd = accept(sock, 0, 0)) < 0)
		return error_log("socket_accept.accept: %s", sys_errlist[errno]);

	setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	fcntl(fd, F_SETFL, O_NDELAY);
	link_fd(fd);
	return fd;
}

int
socket_open(char *host, int port)
{
	struct sockaddr_in server;
	struct hostent *hp;
	char hostname[80];
	int sock;
	int linger = 0;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return error_log("socket_open.socket: %s", sys_errlist[errno]);

	server.sin_family = AF_INET;

	if(host == NULL) {
		if(gethostname(&hostname, 80) < 0)
			return error_log("socket_open.gethostname: %s", sys_errlist[errno]);
		host = hostname;
	}

	if((hp = gethostbyname(host)) == NULL) {
		error_log("socket_open.gethostbyname:");
		return error_log("could not find \"%s\" in the hosts database\n", host);
	}

	bcopy((char*)hp->h_addr, (char*)&server.sin_addr, hp->h_length);

	server.sin_port = htons(port);

	if(connect(sock, (struct sockaddr*)&server, sizeof server) < 0)
		return error_log("socket_open.connect: %s", sys_errlist[errno]);

	setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
	fcntl(sock, F_SETFL, O_NDELAY);
	link_fd(sock);
	return sock;
}

int
socket_read_pending(int fd)
{
	struct fd_list *fdl = find_fd(fd);

	if(fd == ERROR) {
		error_log("socket_read_line: closed fd supplied");
		return sockERROR;
	}

	if(fdl == NULL)
		return FALSE;
	if(fdl->save[0] != 0)
		return TRUE;
	return FALSE;
}

int
socket_read_line(int fd, char *line, int len, int timeout)
{
	int result = socket_read_raw_line(fd, line, len, timeout);
	
	if(result == sockOK) {
		char *p = (char*)index(line, '\n');
		if(p != NULL)
			*p = 0;
	}
	return result;
}

int
socket_read_raw_line(int fd, char *line, int len, int timeout)
{
	struct timeval t;
	int fdlimit = fd+1;
	fd_set ready;
	int cnt = 0;
	char inbuf[256];
	char *p = inbuf, *q = line;
	struct fd_list *fdl = find_fd(fd);

	if(fd == ERROR) {
		error_log("socket_read_line: closed fd supplied");
		return sockERROR;
	}

	if(fdl == NULL)
		fdl = link_fd(fd);

			/* begin by checking that we have no more in the save buf.
			 * if we do then preload it into line.
			 */

	if(fdl->save[0] != 0) {
		strcpy(inbuf, fdl->save);
		fdl->save[0] = 0;
		cnt = strlen(inbuf);

		while(cnt) {
			if(len == 0) {
				*q = 0;
	if(socket_watch == fd)
		socket_print('m', line);
				if(*p) {
					strcpy(fdl->save, p);
				}
				return sockMAXLEN;
			}

			if(*p == '\r') {
				p++;
				continue;
			}

			if(*p == '\n') {
				*q++ = *p++;
				*q = 0;
	if(socket_watch == fd)
		socket_print('r', line);
				if(*p) {
					strcpy(fdl->save, p);
				}
				return sockOK;
			}

			*q++ = *p++;
			*q = 0;
			cnt--;
			len--;
		}
	}

	FD_ZERO(&ready);
	FD_SET(fd, &ready);
	bzero(&t, sizeof(t));
	t.tv_sec = timeout;

	while(TRUE) {
		int reason = sockERROR;

		switch(select(fdlimit, &ready, NULL, NULL, &t)) {
		case ERROR:
			error_log("socket_read_line: select(): %s", sys_errlist[errno]);
			return sockERROR;
		case 0:
#if 0
			error_log("socket_read_line: select(): TIMEOUT");
#endif
			return sockTIMEOUT;

		default:
				/* attempt to read the maximum length allowed in the supplied
				 * buffer and null terminate it.
				 */
			cnt = read(fd, inbuf, 255);

			if(cnt == 0 || cnt == ERROR)
				return sockERROR;
			inbuf[cnt] = 0;

			p = inbuf;
			while(cnt) {
				if(len == 0) {
					*q = 0;
	if(socket_watch == fd)
		socket_print('M', line);
					if(*p) {
						strcpy(fdl->save, p);
					}
					return sockMAXLEN;
				}

				if(*p == '\r') {
					p++;
					continue;
				}

				if(*p == '\n') {
					*q++ = *p++;
					*q = 0;
					if(*p)
						strcpy(fdl->save, p);
	if(socket_watch == fd)
		socket_print('R', line);
					return sockOK;
				}

				*q++ = *p++;
				*q = 0;
				cnt--;
				len--;
			}
		}
	}
}


int
socket_write(int fd, char *buf)
{
#ifdef _DEBUG
	if(fd == ERROR)
		return error_log("socket_write: closed fd supplied");
#endif

	socket_raw_write(fd, buf);
	socket_raw_write(fd, "\n");
	return OK;
}

int
socket_raw_write(int fd, char *buf)
{
	int nleft, nwritten;
	char *p = buf;

#ifdef _DEBUG
	if(fd == ERROR)
		return error_log("socket_raw_write: closed fd supplied");
#endif
	if(socket_watch == fd)
		socket_print('W', buf);
	nleft = strlen(buf);
	while(nleft) {
		if((nwritten = write(fd, p, nleft)) == ERROR) {
			switch(errno) {
			case EWOULDBLOCK:
#if EAGAIN != EWOULDBLOCK
			case EAGAIN:
#endif
				nwritten = 0;
				break;
			case EPIPE:
				return ERROR;
			default:
				return error_log("socket_raw_write: %s", sys_errlist[errno]);
			}
		}
		nleft -= nwritten;
		p += nwritten;
	}
	return OK;
}

int
socket_close(int fd)
{
#ifdef _DEBUG
	if(fd == ERROR)
		return error_log("socket_close: closed fd supplied");
#endif
	if(socket_watch == fd)
		socket_watch = ERROR;
	unlink_fd(fd);
	close(fd);
	return OK;
}

