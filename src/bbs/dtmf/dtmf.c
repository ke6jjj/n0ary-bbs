#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/wait.h>

#include "config.h"
#include "tools.h"

#define KBD				FALSE
#define RESPAWN			FALSE

#define IDLE			0
#define INTERDIGIT		1

char
	*pgm;

int
#if RESPAWN
	respawn_t = 60*30,
#endif
	interdigit_t = 3;

extern int talk(char *);

void
respawn(void)
{
#if RESPAWN
	execl(pgm, pgm, NULL);
#endif
}

int
open_dtmf_port(void)
{
	char b[20];
	struct termios tt;
	int fd;

	if((fd = open(DTMF_DEVICE, O_RDONLY)) < 0) {
		printf("Failed open\n");
		exit(1);
	}

#ifdef HAVE_TERMIOS
	if (tcgetattr(fd, &tt) < 0) {
		perror("tcgetattr");
		exit(1);
	}
#else
	if(ioctl(fd, TCGETS, &tt)) {
		perror("ioctl TCGETS");
		exit(1);
	}
#endif

	tt.c_iflag = IGNCR;
	tt.c_oflag = OPOST | ONLCR;
	tt.c_cflag = B1200 | CS8 | CSTOPB | CREAD;
	tt.c_lflag = 0;

#ifdef HAVE_TERMIOS
	if (tcsetattr(fd, TCSANOW, &tt) < 0) {
		perror("tcsetattr");
		exit(1);
	}
#else
	if(ioctl(fd, TCSETS, &tt)) {
		perror("ioctl TCSETS");
		exit(1);
	}
#endif

	return fd;
}

void
set_nonblocking(int fd)
{
	if(fcntl(fd, F_SETFL, FNDELAY) < 0) {
		perror("set_nonblocking");
		exit(1);
	}
}

char input[1024];
char *ip;

void
read_fd(int fd)
{
	int cnt;
	if((cnt = read(fd, ip, 80)) < 0)
		perror("read");
	else
		ip += cnt;
	*ip = 0;
}

void
system_cmd(char *s)
{
	char out[256];
	int pid = fork();

	if(pid == 0) {
		execl("/bbs/bin/system", "system", s, NULL);
		exit(1);
	}
	waitpid(pid, NULL, 0);
}

void
bbs_cmd(char *s)
{
	char out[256];
	int pid = fork();

	if(pid == 0) {
		execl("/bbs/bin/remote", "remote", s, NULL);
		exit(1);
	}
	waitpid(pid, NULL, 0);
}

void
metcon_cmd(char *s)
{
	char cmd[256];
	char n = *s++;

	if(n < '0' || n > '7')
		return;

	switch(*s++) {
	case '0':
		sprintf(cmd, "/bbs/bin/metcon off %c", n);
		system(cmd);
		if(*s == '1') {
			sleep(5);
			sprintf(cmd, "/bbs/bin/metcon on %c", n);
			system(cmd);
		}
		break;
	case '1':
		sprintf(cmd, "/bbs/bin/metcon on %c", n);
		system(cmd);
		if(*s == '0') {
			sleep(5);
			sprintf(cmd, "/bbs/bin/metcon off %c", n);
			system(cmd);
		}
		break;
	}
}

configure_remote(char *s)
{
	char out[256];
	int value;

	switch(*s++) {
	case '1':
		value = atoi(s);
		if(value)
			interdigit_t = value;
		sprintf(out, "interdigit time is %d seconds", interdigit_t);
		break;
#if RESPAWN
	case '2':
		value = atoi(s);
		if(value)
			respawn_t = value * 60;
		sprintf(out, "respawn time is %d minutes", respawn_t/60);
		break;
	case '3':
		sprintf(out,
			"interdigit time is %d seconds,\n"
			"respawn time is %d minutes",
			interdigit_t, respawn_t/60);
		break;
#endif
	case '9':
		talk("respawn remote now");
		respawn();
		break;
	}
	talk(out);
}

void
issue_cmd(char *s)
{
	if(!strncmp(s, "11", 2))
		bbs_cmd(&s[2]);
	else if(!strncmp(s, "12", 2))
		configure_remote(&s[2]);
	else if(!strncmp(s, "22", 2))
		system_cmd(&s[2]);
	else if(!strncmp(s, "304", 3))
		metcon_cmd(&s[3]);
	else if(!strncmp(s, "999", 3))
		system("/bbs/bin/tst");
}

int
main(int argc, char *argv[])
{
	int cnt;
	int dtmf; 
	int fdlimit;
	struct timeval timeout;
	int state = IDLE;

	pgm = argv[0];
	test_host(DTMF_HOST);

#ifndef DEBUG
	daemon();
#endif

	fdlimit = getdtablesize();
	dtmf = open_dtmf_port();

	set_nonblocking(dtmf);
#if KBD
	set_nonblocking(0);
#endif

	ip = input;

	while(TRUE) {
		fd_set ready;

		FD_ZERO(&ready);
		FD_SET(dtmf, &ready);
#if KBD
		FD_SET(0, &ready);
#endif

		timeout.tv_usec = 0;
		switch(state) {
		case IDLE:
#if RESPAWN
			timeout.tv_sec = respawn_t;
			cnt = select(fdlimit, &ready, (fd_set*)0, (fd_set*)0, &timeout);
#else
			cnt = select(fdlimit, &ready, (fd_set*)0, (fd_set*)0, NULL);
#endif
			break;
		case INTERDIGIT:
			timeout.tv_sec = interdigit_t;
			cnt = select(fdlimit, &ready, (fd_set*)0, (fd_set*)0, &timeout);
			break;
		}


		if(cnt < 0) {
			perror(argv[0]);
			exit(1);
		}

		if(cnt == 0) {
			switch(state) {
			case INTERDIGIT:
				issue_cmd(input);
				ip = input;
				state = IDLE;
				break;
			case IDLE:
#if RESPAWN
				respawn();
#endif
				break;
			}
			continue;
		}

		if(FD_ISSET(dtmf, &ready)) {
			read_fd(dtmf);
			state = INTERDIGIT;
			continue;
		}

#if KBD
		if(FD_ISSET(0, &ready)) {
			read_fd(0);
			state = INTERDIGIT;
			continue;
		}
#endif
	}
}
