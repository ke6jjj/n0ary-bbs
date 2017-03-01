#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

int
wait_for_line(int fd, char *match)
{
	if((re_comp(match)) != NULL)
		return ERROR;

	while(TRUE) {
		char buf[1024];
		switch(socket_read_line(fd, buf, 1024, 60)) {
		case sockERROR:
			return ERROR;
		case sockTIMEOUT:
			return TRUE;
		case sockOK:
log_f("wpd", "r:", buf);
			if(re_exec(buf))
				return OK;
		}
	}
}

int
match(int fd, char *s)
{
    char buf[256];

	switch(socket_read_line(fd, buf, 1024, 60)) {
	case sockERROR:
		printf("Error reading while waiting for %s\n", s);
		exit(1);
	case sockTIMEOUT:
		printf("Timeout in match waiting for %s\n", s);
		exit(1);
	case sockOK:
		if(!strncmp(buf, s, strlen(s)))
			break;
		printf("Got: %s\n", buf);
		return ERROR;
	}
	
	return OK;
}

int
msg_generate(struct smtp_message *msg)
{
	int to_bbs[2], to_gate[2];
	char buf[SMTP_BUF_SIZE];
	struct text_line *line, *body;

	if(pipe(to_bbs) < 0)
		return ERROR;

	if(pipe(to_gate) < 0) {
		close(to_bbs[0]);
		close(to_bbs[1]);
		return ERROR;
	}

	if(fork() == 0) {
		char cmd[256];
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

	body = msg->body;
	while(body) {
		int csize;

log_f("wpd", "M:", "waiting on prompt >");
    	if(wait_for_line(to_gate[0], ".*>$") != OK) {
			printf("expected >\n");
			return ERROR;
		}

		sprintf(buf, "SP %s < %s\n", msg->rcpt->s, msg->from->s);
log_f("wpd", "M:", buf);
		write(to_bbs[1], buf, strlen(buf));

		if(match(to_gate[0], "OK") == ERROR) {
			printf("expected OK\n");
			return ERROR;
		}

			/* would wait for the OK here */

		if(msg->sub[0] == 0)
			sprintf(buf, "[no subject]\n");
		else
			sprintf(buf, "%s\n", msg->sub);
		write(to_bbs[1], buf, strlen(buf));

		line = msg->header;
		while(line) {
			/* special, catch the @dir and change it to @bbs,
			 * we must hide this from the user!
			 */
			char *p = strstr(line->s, "@dir");
			if(p != NULL) {
				*(++p) = 'b';
				*(++p) = 'b';
				*(++p) = 's';
			}

			if(line->s[0] == '/' || line->s[0] == '.' || line->s[0] == '')
				sprintf(buf, ">%s\n", line->s);
			else
				sprintf(buf, "%s\n", line->s);
			write(to_bbs[1], buf, strlen(buf));
			NEXT(line);
		}

		write(to_bbs[1], "\n", 1);

		csize = 0;
		while(body) {
			sprintf(buf, "%s\n", body->s);
			write(to_bbs[1], buf, strlen(buf));
			csize += strlen(buf);
			if(csize > msg->max_size)
				break;
			NEXT(body);
		}
		write(to_bbs[1], "/EX\n", 4);
	}
	write(to_bbs[1], "EXIT\n", 5);
	close(to_gate[0]);
	close(to_bbs[1]);
	wait3(NULL, WNOHANG, NULL);
	return OK;
}

int
disp_update(struct active_processes *ap, struct smtp_message *msg)
{
	char buf[SMTP_BUF_SIZE];
	struct text_line *line, *body;

	if(ap == NULL)
		return ERROR;

	body = msg->body;
	while(body) {
		int csize;

		sprintf(buf, "SP %s < %s\n", msg->rcpt->s, msg->from->s);
		socket_raw_write(ap->fd, buf);
		
		if(msg->sub[0] == 0)
			sprintf(buf, "[no subject]\n");
		else
			sprintf(buf, "%s\n", msg->sub);
		socket_raw_write(ap->fd, buf);

		line = msg->header;
		while(line) {
			/* special, catch the @dir and change it to @bbs,
			 * we must hide this from the user!
			 */
			char *p = strstr(line->s, "@dir");
			if(p != NULL) {
				*(++p) = 'b';
				*(++p) = 'b';
				*(++p) = 's';
			}

			if(line->s[0] == '/' || line->s[0] == '.' || line->s[0] == '')
				sprintf(buf, ">%s\n", line->s);
			else
				sprintf(buf, "%s\n", line->s);
			socket_raw_write(ap->fd, buf);
			NEXT(line);
		}

		socket_raw_write(ap->fd, "\n");

		csize = 0;
		while(body) {
			sprintf(buf, "%s\n", body->s);
			socket_raw_write(ap->fd, buf);
			csize += strlen(buf);
			if(csize > msg->max_size)
				break;
			NEXT(body);
		}
		socket_raw_write(ap->fd, "/EX\n");
	}
	return OK;
}
