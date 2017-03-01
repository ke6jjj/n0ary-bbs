#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "user.h"
#include "tokens.h"
#include "message.h"

#include "system.h"
#include "smtp.h"

scan_for_to(struct text_line **tl, char *rcpt)
{
	int found = FALSE;

	while(*tl) {
		if(!strncmp((*tl)->s, "R:", 2) || (*tl)->s[0] == 0) {
			*tl = (*tl)->next;
			continue;
		}

		ToUpper((*tl)->s[0]);
		ToUpper((*tl)->s[1]);
		if(!strncmp((*tl)->s, "TO:", 3)) {
			char *p = &((*tl)->s[3]);
			NextChar(p);
			if(*p == 0)
				return ERROR;
			sprintf(rcpt, "%s %s", rcpt, p);
			found = TRUE;
			break;
		}
		break;
	}

	if(found)
		return OK;
	return ERROR;
}

int
msg_mail(int msgnum, char *name)
{
	struct msg_dir_entry *m = msg_locate(msgnum);
	struct text_line *tl;
	struct smtp_message *smtpmsg = malloc_struct(smtp_message);
	char rcpt[1024];
	char from[1024], to[1024];
	int result;

	if(m == NULL)
		return ERROR;
	msg_ReadBodyBy(m, m->to.name.str);
	tl = m->body;

	strcpy(rcpt, name);

	if(rcpt[0] == 0) {
		if(scan_for_to(&tl, rcpt) != OK)
			return ERROR;
	} else {
		while(tl) {
			if(strncmp(tl->s, "R:", 2) && tl->s[0] != '\n')
				break;
			NEXT(tl);
		}
	}

	if(m->from.at.str[0])
		sprintf(from, "%s%%%s",
			m->from.name.str, m->from.at.str);
	else
		sprintf(from, "%s", m->from.name.str);

	sprintf(to, "%s@%s", m->to.name.str, m->to.at.str);

	smtp_add_recipient(smtpmsg, rcpt, SMTP_REAL);
	smtp_add_sender(smtpmsg, from);
	smtp_add_recipient(smtpmsg, to, SMTP_ALIAS);
	while(tl) {
		smtp_add_body(smtpmsg, tl->s);
		NEXT(tl);
	}
	smtp_set_subject(smtpmsg, m->sub);
	smtp_log_enable();
	if((result = smtp_send_message(smtpmsg)) == ERROR) {
		error_log("msg_mail: smtp_send_message failed");
		error_print();
		smtp_print_message(smtpmsg);
	}
	smtp_log_disable();
	smtp_free_message(smtpmsg);
	free(smtpmsg);

	return result;
}


struct smtp_message
	*smtpmsg = NULL;

int smtp_message_size;

int
msg_smtp_cmd(char *buf) {
	if(smtp_message_size > Bbs_Import_Size)
		return ERROR;

	smtp_add_body(smtpmsg, buf);
	smtp_message_size += strlen(buf);
	return TRUE;
}

int
msg_smtp_body(char *buf) {
	smtp_add_body(smtpmsg, buf);
	smtp_message_size += strlen(buf);
	return OK;
}

int
msg_smtp_term(void) {
	smtp_add_body(smtpmsg, "/EX");
	smtp_message_size += 4;
	return OK;
}

void
msg_fwd_by_smtp(struct System *sys)
{
	int result;

	ISupportHloc = TRUE;
	ISupportBID = TRUE;
	ImBBS = TRUE;

	smtpmsg = malloc_struct(smtp_message);
	smtp_add_recipient(smtpmsg, sys->connect, SMTP_REAL);
	smtp_add_sender(smtpmsg, Bbs_Call);

	smtp_message_size = 0;

	result = msg_forward(msg_smtp_cmd, msg_smtp_body, msg_smtp_term);

		/* unlike the tnc and tcp forwarding there is no reason
		 * to "initiate" this connect if the forward file is empty.
		 * It is not possible to return an ERROR condition from 
		 * msg_forward because that will alter the behavior of
		 * the other forwarding mechanisms. We must therefore use
		 * a trick now to detect no outgoing messages.
		 */
	if(smtpmsg->body != NULL) {

		if(result == OK) {
			char buf[80];
			sprintf(buf, "Forwarding from %s", Bbs_Call);
			smtp_set_subject(smtpmsg, buf);
			smtp_log_enable();
			if(smtp_send_message(smtpmsg) == ERROR) {
				error_log("msg_fwd_by_smtp: smtp_send_message failed");
				error_print();
				smtp_print_message(smtpmsg);
			}
			smtp_log_disable();
		}
	}

	smtp_free_message(smtpmsg);
	free(smtpmsg);

	ISupportHloc = FALSE;
	ISupportBID = FALSE;
	ImBBS = FALSE;
	return;
}

