#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"

static int socket_watch = ERROR;

void
socket_watcher(int sock_fd)
{
	socket_watch = sock_fd;
}

static void
socket_print(char c, const char *s)
{
	printf("%c(%zu):", c, strlen(s));
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
socket_listen(const char *bind_addr, int *port)
{
	struct sockaddr_in server;
	socklen_t length = sizeof(server);
	int sock = ERROR;
	int opt;

	server.sin_family = AF_INET;

	/*
	 * Choose the network interface(s) to listen on by consulting the
	 * supplied bind address, which will be one of three things:
	 *
	 *  1. If the address is "*" then the caller would like to bind
	 *     to all IP interfaces on this machine.
	 *
	 *  2. Otherwise, if the address is non-NULL, treat it as an
	 *     IP address and bind specifically to the interface with that
	 *     address.
	 *
	 *  3. Otherwise, and finally, if the address is NULL, bind only
	 *     to the loopback interface.
	 */
	if (bind_addr == NULL) {
		/* No address provided. Bind locally */
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	} else if (strcmp(bind_addr, "*") == 0) {
		/* Bind to all (the old default behavior for this code */
		server.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		/* Bind to a specific address, in dotted quad form */
		if (inet_aton(bind_addr, &server.sin_addr) != 1) {
			/* Badly formatted address */
			return error_log(
				"socket_listen.inet_aton(%s)): "
				"Bad IP address", bind_addr);
		}
	}

	server.sin_port = htons(*port);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return error_log("socket_listen.socket: %s",
			sys_errlist[errno]);

	opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		return  error_log("socket_listen.setsockopt(REUSEADDR): %s",
			sys_errlist[errno]);

	if(bind(sock, (struct sockaddr *)&server, length) < 0)
		return  error_log("socket_listen.bind: %s",
			sys_errlist[errno]);

	if(getsockname(sock, (struct sockaddr *)&server, &length) < 0)
		return error_log("socket_listen.getsockname: %s",
			sys_errlist[errno]);

	listen(sock, 5);

	*port = ntohs(server.sin_port);
	return sock;
}

int
socket_accept(int sock)
{
	int fd;
	int linger = 0;

	if((fd = accept(sock, 0, 0)) < 0)
		return error_log("socket_accept.accept: %s",
			sys_errlist[errno]);

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
	int sock;
	int linger = 0;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return error_log("socket_open.socket: %s", sys_errlist[errno]);

	server.sin_family = AF_INET;

	if(host == NULL) {
		server.sin_addr.s_addr =  htonl(INADDR_LOOPBACK);
	} else {
		if((hp = gethostbyname(host)) == NULL) {
			error_log("socket_open.gethostbyname:");
			return error_log(
				"could not find \"%s\" in the hosts "
				"database\n", host);
		}

		bcopy((char*)hp->h_addr, (char*)&server.sin_addr,
			hp->h_length);
	}

	server.sin_port = htons(port);

	if(connect(sock, (struct sockaddr*)&server, sizeof server) < 0)
		return error_log("socket_open.connect: %s",
			sys_errlist[errno]);

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
			error_log("socket_read_line: select(): %s",
				sys_errlist[errno]);
			return sockERROR;
		case 0:
#if 0
			error_log("socket_read_line: select(): TIMEOUT");
#endif
			return sockTIMEOUT;

		default:
			/* attempt to read the maximum length allowed in the
			 * supplied buffer and null terminate it.
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
socket_raw_write(int fd, const char *buf)
{
	size_t len = strlen(buf);
	return socket_raw_write_n(fd, buf, len);
}

int
socket_raw_write_n(int fd, const char *buf, size_t len)
{
	ssize_t nleft, nwritten;
	const char *p = buf;

#ifdef _DEBUG
	if(fd == ERROR)
		return error_log("socket_raw_write_n: closed fd supplied");
#endif
	if(socket_watch == fd)
		socket_print('W', buf);
	nleft = len;
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
				return error_log("socket_raw_write_n: %s",
					sys_errlist[errno]);
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

/*
 * Given a string [<addr>:]portnum parse it into a host:port pair suitable
 * for socket_listen(). The host portion of the spec, if present, will be
 * copied into the "host_buf" buffer (up to host_buf_sz characters) and
 * a pointer to it will be provided on return in "host".
 *
 * If no address is detected, host will be set to NULL.
 * If no port is detected, function will return -1.
 *
 * Returns 0 on success.
 */
int
socket_parse_bindspec(const char *spec, char *host_buf, size_t host_buf_sz,
	int *port, char **host)
{
	char *colon, *end;
	ptrdiff_t cnt;

	/*
	 * If there's a colon in the string, use it to split the host
	 * portion from the port portion.
	 */
	if ((colon = index(spec, ':')) != NULL) {
		/*
		 * There's a colon, capture the IP/host.
		 */
		cnt = colon - spec;
		if (cnt > host_buf_sz - 1) {
				return error_log("socket_parse_bindspec: "
					"Host address too big '%s'", spec);
		}
		strncpy(host_buf, spec, cnt);
		host_buf[cnt] = '\0';

		/* Return the host portion in caller's pointer */
		*host = host_buf;

		/* Continue processing to find port number */
		spec = colon + 1;
	} else {
		/* No host provided, continue processing with port number */
		*host = NULL;
	}

	*port = (int) strtoul(spec, &end, 10);
	if (end == spec) {
		/* Parse error, no port given! */
		return -1;
	}

	return 0;
}
