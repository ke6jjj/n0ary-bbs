#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "smtp.h"

int
match(int fd, char *s)
{
    char buf[80];
    read(fd, buf, 80);
    if(!strncmp(buf, s, strlen(s)))
        return OK;
    return ERROR;
}

int
msg_generate(struct smtp_message *msg)
{
	int to_bbs[2], to_gate[2];
	char buf[SMTP_BUF_SIZE];
	struct text_line *line, *from = msg->from;

	if(pipe(to_bbs) < 0)
		return ERROR;

	if(pipe(to_gate) < 0) {
		close(to_bbs[0]);
		close(to_bbs[1]);
		return ERROR;
	}

	if(fork() == 0) {
		close(to_bbs[1]);
		close(0);
		dup(to_bbs[0]);
		close(to_bbs[0]);

		close(to_gate[0]);
		close(1);
		dup(to_gate[1]);
		close(to_gate[1]);

		execl("/bbs/bin/bbs", "bbs", "gatewy", "console", 0);
		exit(1);
	}

	close(to_bbs[0]);
	close(to_gate[1]);

	match(to_gate[0], ">");

	sprintf(buf, "SP %s < %s\n", msg->rcpt->s, from->s);
	write(to_bbs[1], buf, strlen(buf));
	NEXT(from);
		
	match(to_gate[0], "OK");
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

		if(line->s[0] == '/' || line->s[0] == '.' || line->s[0] == '\x1a')
			sprintf(buf, ">%s\n", line->s);
		else
			sprintf(buf, "%s\n", line->s);
		write(to_bbs[1], buf, strlen(buf));
		NEXT(line);
	}

	write(to_bbs[1], "\n", 1);

	line = msg->body;
	while(line) {
		sprintf(buf, "%s\n", line->s);
		write(to_bbs[1], buf, strlen(buf));
		NEXT(line);
	}
	write(to_bbs[1], "/EX\n", 4);
	match(to_gate[0], ">");
	write(to_bbs[1], "EXIT\n", 5);
	close(to_gate[0]);
	close(to_bbs[1]);
	return OK;
}

