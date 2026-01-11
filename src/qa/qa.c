#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#if HAVE_REGCOMP
#include <regex.h>
#endif /* HAVE_REGCOMP */

#include "c_cmmn.h"
#include "tools.h"

#define TRACE_FDS 0

FILE *log_fp;

#define MAXSOCKETS 20

time_t time_base = 0;

struct text_line *PendCmd = NULL;

struct Sockets {
	int sock;
	int fd;
} Socket[MAXSOCKETS];

int
	RegExp = FALSE,
	test_mode = FALSE,
	child = 0;

static void usage(const char *prog);

int
	exec_listen(char *s),
	compare_files(char *s),
	execute(char *s),
	fork_exec(char *s),
	open_socket(char *s),
	close_socket(char *s),
	delay(char *s),
	receive_till(char *s),
	receive(char *s),
	receive_till_exp(char *s),
	receive_exp(char *s),
	issue(char *s),
	issue_from_file(char *s),
	receive_cmp_file(char *s),
	sum_file(char *s),
	read_script_file(char *s),
	install_pending_cmd(char *s),
	verbose(char *s),
	stamp(char *s),
	finished(char *s);

struct script_cmds {
	char *txt;
	int (*func)(char *s);
} Cmds[] = {
	{ "EXEC",	execute },
	{ "FORK",	fork_exec },
	{ "SOCK",	open_socket },
	{ "LISTEN",	exec_listen },
	{ "CLOSE",	close_socket },
	{ "DELAY",	delay },
	{ "WAIT",	receive_till },
	{ "RECV",	receive },
	{ "RWAIT",	receive_till_exp },
	{ "RRECV",	receive_exp },
	{ "FRECV",	receive_cmp_file },
	{ "SEND",	issue },
	{ "STAMP",	stamp },
	{ "EXIT",	finished },
	{ "COMPARE",	compare_files },
	{ "FSEND",	issue_from_file },
	{ "FSUM",	sum_file },
	{ "RUN",	read_script_file },
	{ "ONEXIT",	install_pending_cmd },
	{ "VERBOSE",	verbose },
	{ NULL,		NULL }};

int
read_script_file(char *fn)
{
	char buf[1024];
	int doit = TRUE;
	FILE *fp = fopen(fn, "r");
	struct text_line *pending_list = PendCmd;

	PendCmd = NULL;

	if(fp == NULL) {
		printf("Could not open script file %s for reading\n", fn);
		fprintf(log_fp, "Could not open script file %s for reading\n", fn);
		exit(1);
	}

	while(fgets(buf, 1024, fp)) {
		char *s, *p = buf;
		struct script_cmds *cmd = &Cmds[0];

		buf[strlen(buf)-1] = 0;

		if(*p == 0)
			continue;

		if(*p == '#') {
			p++;
			if(*p == '#') {
				p++;
				printf("%s\n", p);
				fprintf(log_fp, "%s\n", p); fflush(log_fp);
			}
			continue;
		}

		s = get_string(&p);
		if(doit == FALSE)
			if(!strcmp(s, "START")) {
				doit = TRUE;
				continue;
			}

		if(!strcmp(s, "STOP"))
			doit = FALSE;
		if(doit == FALSE)
			continue;

		while(cmd->txt) {
			if(!strcmp(cmd->txt, s)) {
				if(cmd->func(p) != OK) {
					textline_free(PendCmd);
					PendCmd = pending_list;
					return ERROR;	
				}
				break;
			}
			cmd++;
		}
		if(cmd->txt == NULL) {
			printf("unknown command %s\n", buf);
			fprintf(log_fp, "unknown command %s\n", buf);
			exit(2);
		}
	}
	fclose(fp);
	textline_free(PendCmd);
	PendCmd = pending_list;

#if TRACE_FDS
	{
		int i;
		printf("Open fds: [ ");
		for(i=0; i<MAXSOCKETS; i++)
			if(Socket[i].fd != ERROR)
				printf("%d ", i);
		printf("]\n");
	}
#endif
	return OK;
}

void
execute_pending(char *p)
{
	char *s;
	struct script_cmds *cmd = &Cmds[0];

	s = get_string(&p);
	while(cmd->txt) {
		if(!strcmp(cmd->txt, s)) {
			cmd->func(p);
			return;	
		}
		cmd++;
	}
	if(cmd->txt == NULL) {
		printf("unknown command %s\n", p);
		fprintf(log_fp, "unknown command %s\n", p);
		exit(2);
	}
}

int
main(int argc, char *argv[])
{
	int c;
#ifdef SUNOS
	extern int optind;
#endif
	struct text_line *tl;
	char *log_path = "/dev/null", *prog = argv[0];

	while((c = getopt(argc, argv, "tl:h?")) != -1) {
		switch(c) {
		case 't':
			test_mode = TRUE;
			break;
		case 'l':
			log_path = optarg;
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			exit(1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) {
		usage(prog);
		exit(1);
	}

	for(c=0; c<MAXSOCKETS; c++) {
		Socket[c].fd = ERROR;
		Socket[c].sock = ERROR;
	}

	log_fp = fopen(log_path, "w");

	read_script_file(argv[0]);

	tl = PendCmd;
	while(tl) {
		execute_pending(tl->s);
		NEXT(tl);
	}
	textline_free(PendCmd);

	fclose(log_fp);
	return 0;
}

int
get_line(char *buf, int fd, int len, int timeout)
{
	struct timeval t;
	int fdlimit = fd + 1;

	if(fd == ERROR) {
		printf("attempt to read from a closed fd\n");
		return ERROR;
	}

	while(TRUE) {
		int result;
		fd_set ready;

		FD_ZERO(&ready);
		FD_SET(fd, &ready);
		
		t.tv_sec = timeout*3;
		t.tv_usec = 0;

		if((result = select(fdlimit, &ready, NULL, NULL, &t)) < 0) {
			perror("get_line(select)");
			exit(1);
		}
		
		if(result == 0)
			return ERROR;

        if(FD_ISSET(fd, &ready)) {
    		char *c = buf;
			int cnt = 0;

    		while(TRUE) {
        		if(read(fd, c, 1) <= 0) {
					perror("get_line(read))");
					exit(1);
				}
				if(*c == '\r')
					continue;
		
        		if(*c == '\n') {
					*c = 0;
					fprintf(log_fp, "<%s\n", buf); fflush(log_fp);
            		break;
				}
				c++;
				if(++cnt >= len)
					break;
    		}
			return cnt;
        }
	}
}

int
stamp(char *s)
{
	time_t t = time(NULL);
	char now[80];
	char buf[80];

	strcpy(now, ctime(&t));
	now[strlen(now)-1] = 0;

	if(*s) {
		time_t delta = t - time_base;
		time_t hours, minutes, seconds;

		hours = delta / 3600;
		delta = delta % 3600;
		minutes = delta / 60;
		seconds = delta % 60;

		sprintf(buf, "%s  delta= %"PRTMd":%02"PRTMd":%02"PRTMd,
			now, hours, minutes, seconds);
	} else {
		time_base = t;
		strcpy(buf, now);
	}

	printf("[%s]\n", buf);
	fprintf(log_fp, "[%s]\n", buf);
	return OK;
}

int
execute(char *s)
{
	system(s);
	return OK;
}

int
fork_exec(char *s)
{
	if((child = fork()) == 0) {
		system(s);
		exit(0);
	}
	return OK;
}

/* OPEN fd# port# */

int
open_socket(char *s)
{
	char *host = NULL;
	int num = get_number(&s);
	int port = get_number(&s);

	if(*s)
		host = get_string(&s);

	if(Socket[num].fd != ERROR) {
		printf("Socket[%d].fd is not closed\n", num);
		return ERROR;
	}

	if((Socket[num].fd = socket_open(host, port)) == ERROR) {
		printf("Couldn't open socket to daemon\n");
		return ERROR;
	}

#if TRACE_FDS
	printf("%d: socket_open(%d) = %d\n", num, port, Socket[num].fd);
#endif
	return OK;
}

int
exec_listen(char *s)
{
	int num, port;
	char *spec, *host, host_buf[128];

	num = get_number(&s);
	spec = get_string(&s);

	if (socket_parse_bindspec(spec, host_buf, sizeof(host_buf), &port,
		&host) != 0) {
		return ERROR;
	}

	if(Socket[num].sock != ERROR) {
		printf("Socket[%d].sock is not closed\n", num);
		return ERROR;
	}
	if(Socket[num].fd != ERROR) {
		printf("Socket[%d].fd is not closed\n", num);
		return ERROR;
	}


	if((Socket[num].sock = socket_listen(host, &port)) == ERROR) {
		return ERROR;
	}

	fork_exec(s);

	if((Socket[num].fd = socket_accept(Socket[num].sock)) == ERROR) {
		return ERROR;
	}
#if TRACE_FDS
	printf("%d: socket_open(%d) = %d\n", num, port, Socket[num].fd);
#endif
	return OK;
}

int
close_socket(char *s)
{
	char *host = NULL;
	int num = get_number(&s);

#if TRACE_FDS
	printf("%d: closing()\n", num);
#endif

	if(Socket[num].sock != ERROR)
		close(Socket[num].sock);
	if(Socket[num].fd != ERROR)
		close(Socket[num].fd);

	Socket[num].sock = ERROR;
	Socket[num].fd = ERROR;
	return OK;
}

int
delay(char *s)
{
	sleep(atoi(s));
	return OK;
}

void
kill_spaces(char *dest, char *src)
{
	while(*src) {
		if(!isspace(*src))
			*dest++ = *src;
		src++;
	}
	*dest = 0;
}

int
compare_string(char *p, char *q)
{
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */

	if(RegExp == FALSE) {
		char buf0[1024], buf1[1024];
		kill_spaces(buf0, p);
		kill_spaces(buf1, q);
		return strcmp(buf0, buf1);
	}
#if HAVE_REGCOMP
	if (regcomp(&preg, q, 0) != 0) {
		perror("compare_string(regcomp))");
		exit(2);
	}
#else
	if(re_comp(q)) {
		perror("compare_string(re_comp))");
		exit(2);
	}
#endif /* HAVE_REGCOMP */


#if HAVE_REGCOMP
	ret = regexec(&preg, q, 0, NULL, 0);
	regfree(&preg);
	if (ret == 0)
		return OK;
#else
	if(re_exec(p) == 1)
		return OK;
#endif
	return TRUE;
}

int
receive_till(char *s)
{
	char buf[1024];
	int num = get_number(&s);
	int to = get_number(&s);

	do {
		if(get_line(buf, Socket[num].fd, 1024, to) == ERROR) {
			printf("**** Timeout occured waiting for:\n%s\n", s);
			fprintf(log_fp, "**** Timeout occured waiting for:\n%s\n", s);
			return ERROR;
		}
		if(test_mode)
			printf("RECV %d %d %s\n", num, to, buf);
	} while(compare_string(buf, s));
	return OK;
}
int
receive_till_exp(char *s)
{
	int result;
	RegExp = TRUE;
	result = receive_till(s);
	RegExp = FALSE;
	return result;
}

int
receive(char *s)
{
	char *p, buf[1024];
	int num = get_number(&s);
	int to = get_number(&s);

	if(get_line(buf, Socket[num].fd, 1024, to) == ERROR) {
		printf("**** Timeout occured waiting for:\n%s\n", s);
		fprintf(log_fp, "**** Timeout occured waiting for:\n%s\n", s);
		return ERROR;
	}
	if(test_mode) {
		printf("RECV %d %d %s\n", num, to, buf);
		return OK;
	}

	p = buf;
	NextChar(p);

#if 0
	if(*s)
#endif
		if(compare_string(p, s)) {
			char *p1 = p, *p2 = s;
			printf("**** EXP: [%s]\n", s);
			printf("**** GOT: [%s]\n", p);
			fprintf(log_fp, "**** EXP: [%s]\n", s);
			fprintf(log_fp, "**** GOT: [%s]\n", p);
			if(!RegExp) {
				printf("****   ");
				fprintf(log_fp, "****   ");
				while(*p1 == *p2) {
					p1++; p2++;
					printf(" ");
					fprintf(log_fp, " ");
				}
				printf("Here^\n");
				fprintf(log_fp, "Here^\n");
			}
			return ERROR;
		}
	return OK;
}
int
receive_exp(char *s)
{
	int result;
	RegExp = TRUE;
	result = receive(s);
	RegExp = FALSE;
	return result;
}

int
issue_from_file(char *s)
{
	char buf[1024];
	int num = get_number(&s);
	FILE *fp = fopen(s, "r");

	if(fp == NULL) {
		printf("**** Couldn't open compare file %s\n", s);
		fprintf(log_fp, "**** Couldn't open compare file %s\n", s);
		return ERROR;
	}

	if(test_mode)
		printf("FSEND %d %s\n", num, s);

	while(fgets(buf, 1024, fp)) {
		fprintf(log_fp, ">%s", buf); fflush(log_fp);
		write(Socket[num].fd, buf, strlen(buf));
	}

	fclose(fp);
	return OK;
}

int
receive_cmp_file(char *s)
{
	char buf[1024], fbuf[1024];
	int num = get_number(&s);
	int to = get_number(&s);
	FILE *fp = fopen(s, "r");

	if(fp == NULL) {
		printf("**** Couldn't open compare file %s\n", s);
		fprintf(log_fp, "**** Couldn't open compare file %s\n", s);
		return ERROR;
	}

	while(fgets(fbuf, 1024, fp)) {
		fbuf[strlen(fbuf)-1] = 0;

		if(get_line(buf, Socket[num].fd, 1024, to) == ERROR) {
			printf("**** Timeout occured waiting for:\n%s\n", fbuf);
			fprintf(log_fp, "**** Timeout occured waiting for:\n%s\n", fbuf);
			fclose(fp);
			return ERROR;
		}
		if(test_mode)
			printf("%s\n", buf);
		else
			if(strcmp(buf, fbuf)) {
				char *p1 = buf, *p2 = fbuf;
				printf("**** EXP: [%s]\n", fbuf);
				printf("**** GOT: [%s]\n", buf);
				printf("****   ");
				fprintf(log_fp, "**** EXP: [%s]\n", fbuf);
				fprintf(log_fp, "**** GOT: [%s]\n", buf);
				fprintf(log_fp, "****   ");
				while(*p1 == *p2) {
					p1++; p2++;
					printf(" ");
					fprintf(log_fp, " ");
				}
				printf("Here^\n");
				fprintf(log_fp, "Here^\n");
				fclose(fp);
				return ERROR;
			}
	}
	fclose(fp);
	return OK;
}

int
issue(char *s)
{
	char buf[1024];
	int num = get_number(&s);

	if(Socket[num].fd == ERROR)
		return OK;
	if(test_mode)
		printf("SEND %d %s\n", num, s);

	sprintf(buf, "%s\n", s);
	fprintf(log_fp, ">%s", buf); fflush(log_fp);
	write(Socket[num].fd, buf, strlen(buf));
	return OK;
}

int
finished(char *s)
{
	return TRUE;
}

int
compare_files(char *s)
{
	char fn1[80], fn2[80];
	char buf1[1024], buf2[1024];
	FILE *fp1, *fp2;
	int error = FALSE;
	
	strcpy(fn1, get_string(&s));
	if((fp1 = fopen(fn1, "r")) == NULL) {
		printf("**** Couldn't open compare file %s\n", fn1);
		fprintf(log_fp, "**** Couldn't open compare file %s\n", fn1);
		return ERROR;
	}
	strcpy(fn2, get_string(&s));
	if((fp2 = fopen(fn2, "r")) == NULL) {
		printf("**** Couldn't open compare file %s\n", fn2);
		fprintf(log_fp, "**** Couldn't open compare file %s\n", fn2);
		return ERROR;
	}

	while(fgets(buf1, 1024, fp1)) {
		buf1[strlen(buf1)-1] = 0;
		if(fgets(buf2, 1024, fp2) == 0) {
			printf("**** EXP: [%s]\n", buf1);
			printf("**** GOT: EOF\n");
			printf("****   ");
			fprintf(log_fp, "**** EXP: [%s]\n", buf1);
			fprintf(log_fp, "**** GOT: EOF\n");
			fprintf(log_fp, "****   ");
			error = TRUE;
			break;
		}
		buf2[strlen(buf2)-1] = 0;

		if(strcmp(buf1, buf2)) {
			char *p1 = buf1, *p2 = buf2;
			printf("**** EXP: [%s]\n", buf1);
			printf("**** GOT: [%s]\n", buf2);
			printf("****   ");
			fprintf(log_fp, "**** EXP: [%s]\n", buf1);
			fprintf(log_fp, "**** GOT: [%s]\n", buf2);
			fprintf(log_fp, "****   ");
			while(*p1 == *p2) {
				p1++; p2++;
				printf(" ");
				fprintf(log_fp, " ");
			}
			printf("Here^\n");
			fprintf(log_fp, "Here^\n");
			error = TRUE;
		}
	}

	if(!error)
		if(fgets(buf2, 1024, fp2) != 0) {
			buf2[strlen(buf2)-1] = 0;
			printf("**** EXP: EOF\n");
			printf("**** GOT: [%s]\n", buf2);
			printf("****   ");
			fprintf(log_fp, "**** EXP: EOF\n");
			fprintf(log_fp, "**** GOT: [%s]\n", buf2);
			fprintf(log_fp, "****   ");
			error = TRUE;
		}

	fclose(fp1);
	fclose(fp2);

	return error;
}

int
sum_file(char *s)
{
	unsigned target = (unsigned)get_hexnum(&s);
	char *fn = get_string(&s);
	unsigned c, sum = 0;
	FILE *fp = fopen(fn, "r");

	if(fp == NULL) {
		printf("**** Couldn't open file %s\n", fn);
		fprintf(log_fp, "**** Couldn't open file %s\n", fn);
		return ERROR;
	}

	while((c = fgetc(fp)) != EOF) {
		sum += c;
		sum &= 0xFFFF;
	}

	if(sum != target) {
		printf("**** Sum of %s was %x expected %x\n", fn, sum, target);
		fprintf(log_fp, "**** Sum of %s was %x expected %x\n", fn, sum, target);
		return ERROR;
	}
	return OK;
}

int
install_pending_cmd(char *s)
{
	textline_prepend(&PendCmd, s);	
	return OK;
}

int
verbose(char *s)
{
	uppercase(s);
	if(!strcmp(s, "ON"))
		test_mode = TRUE;
	else
		test_mode = FALSE;
	return OK;
}

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s [-t] [-l <logfile>] <scriptfile>\n", prog);
	fprintf(stderr, "-t Test mode\n");
	fprintf(stderr, "-l Write debug log to <logfile>\n");
}
