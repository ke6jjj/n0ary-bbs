#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#ifndef SABER
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdlib.h>


#include "c_cmmn.h"
#include "tools.h"

int
get_line(int fd, char *p, int cnt)
{
	char *str = p;

	while(TRUE) {
		switch(read(fd, str, 1)) {
		case 0:
			return ERROR;

		case ERROR:
			error_log("get_socket.read(): %s", sys_errlist[errno]);
			error_print_exit(2);
		}

		if(*str == '\n') {
			*str = 0;
			return OK;
		}
		if(*str == '\r') {
			str--;
			cnt++;
		}

		*str &= 0x7F;
		str++;
		if(--cnt == 0) {
			*str = 0;
			return OK;
		}
	}
}

void
putchar_socket(int fd, char *str)
{
	if(write(fd, str, strlen(str)) != strlen(str))
		error_log("putchar_socket.write(): %s", sys_errlist[errno]);
}

void
print_socket(int fd, char *fmt, ...)
{
	va_list ap;
	char buf[4096];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	putchar_socket(fd, buf);
}

int
main(int argc, char *argv[])
{
	char c;
	int port_number = 40001;
	char *str, *host = NULL, *bind_addr = NULL;
	char buf[1024];
	int len;
	fd_set ready;
	int in = 0, out = 1;
	int socket_number = 0;
	int listen_sock = ERROR;
	int listen_fd = ERROR;

	extern char *optarg;
	while((c = getopt(argc, argv, "b:p:s:h:")) != -1) {
		switch(c) {
		case 'b':
			bind_addr = optarg;
			break;
		case 'p':
			port_number = atoi(optarg);
			break;
		case 's':
			socket_number = atoi(optarg);
			break;
		case 'h':
			host = optarg;
			break;
		}
	}

	if((listen_sock = socket_listen(bind_addr, &port_number)) == ERROR)
		error_print_exit(1);

	if(socket_number) {
		if((in = socket_open(host, socket_number)) == ERROR)
			error_print_exit(1);
		out = in;
	}
	write(out, "OK\n", 3);

	while(TRUE) {
		int result;
		int fdlimit;

		FD_ZERO(&ready);
		if(listen_fd == ERROR) {
			FD_SET(listen_sock, &ready);
			fdlimit = listen_sock;
		} else {
			FD_SET(listen_fd, &ready);
			fdlimit = listen_fd;
		}
		FD_SET(in, &ready);
		if(in > fdlimit)
			fdlimit = in;
		fdlimit++;

		result = select(fdlimit, (void*)&ready, NULL, NULL, NULL);

		if(FD_ISSET(listen_sock, &ready)) {
			listen_fd = socket_accept(listen_sock);
			print_socket(out, "CONNECTED\n");
			continue;
		}

		if(FD_ISSET(in, &ready)) {
			char cmd[1024];
			get_line(in, buf, 1024);
			strcpy(cmd, buf);
			uppercase(cmd);
			if(!strcmp(cmd, "SHUTDOWN"))
				break;
			print_socket(listen_fd, "%s\n", buf);
			continue;
		}

		if(FD_ISSET(listen_fd, &ready)) {
			if(get_line(listen_fd, buf, 1024) == ERROR) {
				print_socket(out, "IDLE\n");
				close(listen_fd);
				listen_fd = ERROR;
			} else
				print_socket(out, "%s\n", buf);
		}
	}

	if(listen_fd != ERROR)
		close(listen_fd);
	close(listen_sock);
	if(in)
		close(in);
	return 0;
}
