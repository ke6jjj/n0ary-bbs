#include <stdio.h>

#include "c_cmmn.h"
#include "tools.h"
#include "smtp.h"

static void
send_msg(struct smtp_message *msg)
{
	smtp_send_message(msg);
	smtp_free_message(msg);
	free(msg);
}

static struct smtp_message *
bug_create_msg(char *bbs, char *ver, char *file, int line, char *func, char *cmd)
{
	char buf[256];
	struct smtp_message *msg = malloc_struct(smtp_message);

	if(msg) {
		smtp_add_recipient(msg, "bob@arasmith.com", SMTP_REAL);
#if 0
		smtp_add_recipient(msg, "root", SMTP_REAL);
#endif
		smtp_add_sender(msg, "BBS_BUG");

		sprintf(buf, "Bug report from %s", bbs);
		smtp_set_subject(msg, buf);

		sprintf(buf, "BBS: %s", bbs);
		smtp_add_body(msg, buf);
		sprintf(buf, "Version: %s", ver);
		smtp_add_body(msg, buf);
		sprintf(buf, "File: %s:%d", file, line);
		smtp_add_body(msg, buf);
		sprintf(buf, "Function: %s", func);
		smtp_add_body(msg, buf);
		sprintf(buf, "Command: %s", cmd);
		smtp_add_body(msg, buf);

		smtp_add_body(msg, "");
		smtp_add_body(msg, "Problem:");
	}
	return msg;
}

void
bug_report(char *bbs, char *ver, char *file, int line, char *func, char *cmd, char *s)
{
	struct smtp_message *msg = bug_create_msg(bbs, ver, file, line, func, cmd);

	if(msg == NULL)
		return;
	smtp_add_body(msg, s);
	send_msg(msg);
}

void
bug_report_textline(char *bbs, char *ver, char *file, int line, char *func, char *cmd, 
	struct text_line *tl)
{
	struct smtp_message *msg = bug_create_msg(bbs, ver, file, line, func, cmd);

	if(msg == NULL)
		return;
	while(tl) {
		smtp_add_body(msg, tl->s);
		NEXT(tl);
	}
	send_msg(msg);
}

static struct smtp_message *
prob_create_msg(char *bbs, char *ver, char *cmd)
{
	char buf[256];
	struct smtp_message *msg = malloc_struct(smtp_message);

	if(msg) {
		smtp_add_recipient(msg, "bob@arasmith.com", SMTP_REAL);
		smtp_add_recipient(msg, "root", SMTP_REAL);
		smtp_add_sender(msg, "BBS_PROB");

		sprintf(buf, "Problem report from %s", bbs);
		smtp_set_subject(msg, buf);

		sprintf(buf, "BBS: %s", bbs);
		smtp_add_body(msg, buf);
		sprintf(buf, "Version: %s", ver);
		smtp_add_body(msg, buf);
		sprintf(buf, "Command: %s", cmd);
		smtp_add_body(msg, buf);

		smtp_add_body(msg, "");
		smtp_add_body(msg, "Problem:");
	}
	return msg;
}

void
problem_report(char *bbs, char *ver, char *cmd, char *s)
{
	struct smtp_message *msg = prob_create_msg(bbs, ver, cmd);

	if(msg == NULL)
		return;
	smtp_add_body(msg, s);
	send_msg(msg);
}

void
problem_report_textline(char *bbs, char *ver, char *cmd, struct text_line *tl)
{
	struct smtp_message *msg = prob_create_msg(bbs, ver, cmd);

	if(msg == NULL)
		return;
	while(tl) {
		smtp_add_body(msg, tl->s);
		NEXT(tl);
	}
	send_msg(msg);
}

