#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "smtp.h"
#include "gateway.h"

#define CALLSIGN	0
#define CATEGORY	1

extern char *Gate_Personal_File;

int
match(int fd, char *s)
{
	char buf[256];

	switch(read_line(fd, buf, 256)) {
	case TRUE:  /*timeout*/
		printf("Timeout in match waiting for %s\n", s);
		exit(1);
		
	case ERROR:
		printf("Error reading while waiting for %s\n", s);
		exit(1);

	case OK:
		if(!strncmp(buf, s, strlen(s)))
			break;
		printf("Got: %s\n", buf);
		return ERROR;
	}
	return OK;
}

static int
to_type(char *s)
{
	int pre = 0, num = 0, suf =0, *part = &pre;
	FILE *fp = fopen(Gate_Personal_File, "r");

	if(fp != NULL) {
		char *to, *p;
		char buf[80];
		if((to = (char*)malloc(strlen(s)+1)) == NULL)
			exit(1);
		
		if((p = (char*)strchr(to, '@')) != NULL)
			*p = 0;

		while(fgets(buf, 80, fp)) {
			buf[strlen(buf)-1] = 0;
			uppercase(buf);
			if(!strcmp(to, buf)) {
				free(to);
				return CALLSIGN;
			}
		}
		free(to);
	}

	while(*s && (*s != '@')) {
		if(isalpha(*s))
			(*part)++;
		else {
			if(isdigit(*s)) {
				num++;
				part = &suf;
			} else
				return CATEGORY;
		}
		s++;
	}

	if(((pre > 0) && (pre < 3)) && (num == 1) && ((suf > 0) && (suf < 4)))
		return CALLSIGN;

	return CATEGORY;
}

int
msg_generate(FILE *fp)
{
	int to_bbs[2], to_gate[2];
	char msgtype;
	char from[1024], to[1024], buf[4096];

	fgets(from, 1024, fp);
	fgets(to, 1024, fp);
	to[strlen(to)-1] = 0;
	from[strlen(from)-1] = 0;

	if(strlen(to) == 0)
		return TRUE;

	if(pipe(to_bbs) < 0)
		return ERROR;

	if(pipe(to_gate) < 0) {
		close(to_bbs[0]);
		close(to_bbs[1]);
		return ERROR;
	}

	if(fork() == 0) {
		char cmd[80];

		close(to_bbs[1]);
		close(0);
		dup(to_bbs[0]);
		close(to_bbs[0]);

		close(to_gate[0]);
		close(1);
		dup(to_gate[1]);
		close(to_gate[1]);

		sprintf(cmd, "%s/b_bbs", Bin_Dir);
		execl(cmd, "b_bbs", "-U", "-v", "SMTP", "gatewy", 0);
		exit(1);
	}

	close(to_bbs[0]);
	close(to_gate[1]);


	if(wait_for_line(to_gate[0], ".*>$") != OK) {
		printf("expected >\n");
		return ERROR;
	}

	if(to_type(to) == CATEGORY)
		msgtype = 'B';
	else
		msgtype = 'P';
			
	sprintf(buf, "S%c %s < %s\n", msgtype, to, from);
	write(to_bbs[1], buf, strlen(buf));

	if(match(to_gate[0], "OK") == ERROR) {
		printf("expected OK\n");
		return TRUE;
	}
			/* would wait for the OK here */

	fgets(buf, 1024, fp);
	write(to_bbs[1], buf, strlen(buf));

	while(fgets(buf, 4096, fp)) {
		if(buf[0] == '\n')
			break;
		write(to_bbs[1], buf, strlen(buf));
	}

	write(to_bbs[1], "\n", 1);

	while(fgets(buf, 4096, fp)) {
		if(buf[0] == '.' && buf[1] == 0)
			break;

		if(buf[0] == '/' || buf[0] == '.' || buf[0] == '')
			write(to_bbs[1], ">", 1);
		write(to_bbs[1], buf, strlen(buf));
	}

	write(to_bbs[1], "/EX\n", 4);
	if(wait_for_line(to_gate[0], ".*>$") != OK) {
		printf("expected >\n");
		return ERROR;
	}
	write(to_bbs[1], "EXIT\n", 5);
	sleep(1);
	close(to_gate[0]);
	close(to_bbs[1]);
	wait3(NULL, WNOHANG, NULL);
	return OK;
}

