#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "smtp.h"
#include "gateway.h"

struct term_strings {
	char *str;
	int min, max;
} term[] = {
	{ "EXIT", 2, 4 },
	{ "BYE", 1, 3 },
	{ "QUIT", 1, 4 },
	{ "LOGOUT", 4, 6 },
	{ "LOGOFF", 4, 6 },
	{ NULL, 0, 0 }};

static int
terminate(char *s)
{
	struct term_strings *ts = &term[0];
	char cmd[256];
	int len;

	/* look for occurances of one of the quit commands so that we can
	 * terminate input early */

	strncpy(cmd, s, 60);
	len = strlen(s);

	if(len > 8)
		return FALSE;

	uppercase(cmd);

	while(ts->str) {
		if(ts->max >= len)
			if(ts->min <= len) 
				if(!strncmp(cmd, ts->str, len))
					return TRUE;
		ts++;
	}
	return FALSE;
}

static int
collect_response_till(struct smtp_message *msg, int fd, char *term)
{
	char buf[SMTP_BUF_SIZE];
	char *s;

	if((s = (char*)re_comp(term)) != NULL)
		exit(1);

	do {
		s = buf;
		while(TRUE) {
			if(read(fd, s, 1) <= 0)
				return ERROR;
			if(*s == '\n') {
				*s = 0;
				break;
			}
			s++;
		}
		smtp_add_body(msg, buf);
	} while(re_exec(buf) == FALSE);
	return OK;
}

int
cmd_generate(FILE *fp)
{
	int to_bbs[2], to_gate[2];
	struct smtp_message *reply = malloc_struct(smtp_message);
	char prompt[256];
	char buf[SMTP_BUF_SIZE];
	char from[1024], to[1024], sub[1024], rcpt[1024];
	int result;

	fgets(from, 1024, fp);
		/* if we send a message out and it comes bouncing back it will
		 * look as though it is coming from IPGATE. Let's skip it here
		 */

	if(!strncmp(from, "IPGATE", 6))
		return TRUE;

	fgets(rcpt, 1024, fp);
	fgets(to, 1024, fp);
	fgets(sub, 1024, fp);

	from[strlen(from)-1] = 0;
	rcpt[strlen(rcpt)-1] = 0;
	to[strlen(to)-1] = 0;
	sub[strlen(sub)-1] = 0;

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
		execl(cmd, "b_bbs", "-v", "SMTP", "-c", "1", from, 0);
		
		exit(1);
	}

	close(to_bbs[0]);
	close(to_gate[1]);

	sprintf(prompt, "%s>", from);

	smtp_add_recipient(reply, rcpt, SMTP_REAL);
	smtp_add_sender(reply, to);
	smtp_set_subject(reply, sub);

	collect_response_till(reply, to_gate[0], prompt);

	while(fgets(buf, SMTP_BUF_SIZE, fp)) {
		if(buf[0] == '.' && buf[1] == 0)
			break;

		buf[strlen(buf)-1] = 0;
		if(terminate(buf))
			break;
		smtp_add_body(reply, buf);

		strcat(buf, "\n");
		write(to_bbs[1], buf, strlen(buf));
		result = collect_response_till(reply, to_gate[0], prompt);
		
		if(result == ERROR)
			break;
	}
	if(result == OK) {
		write(to_bbs[1], "EXIT\n", 5);
		sleep(2);
	}
	close(to_gate[0]);
	close(to_bbs[1]);

	if(smtp_send_message(reply) != OK) {
		error_log("cmd_generate: smtp_send_message failed");
		error_print();
		smtp_print_message(reply);
	}
	smtp_free_message(reply);
	free(reply);
	wait3(NULL, WNOHANG, NULL);

	return OK;
}

