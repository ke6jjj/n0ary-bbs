#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "vars.h"

struct generated_message
	*GenMsg = NULL;

/*==========================================================================
 * While the program is busy dealing with another message, reading, sending,
 * etc. it can queue up a message to be sent when the message channel is
 * idle. These routines are used to facilitate this.
 *
 * The calling sequence should go as follows:
 *		buffer_msg_to_user("SYSOP", "text to place in the message\n");
 *		buffer_msg_to_user("N0ARY", "text of message to N0ARY\n");
 *		buffer_msg_to_user("SYSOP", "additional text\n");
 *		send_msg_to_user("N0ARY", "subject goes here", "last line of text\n");
 *		send_msg_to_user("SYSOP", "subject again", "last line\n");
 *
 * The first parameter is used to indicate the message to concatenate the
 * supplied line to. Only one message can be created at any given time to
 * a single person but you can have multiple messages being queued at the
 * same time.
 */

static int
free_genmsg(struct generated_message *genmsg)
{
	struct generated_message *tmp, *gm = GenMsg;
	if(genmsg == GenMsg) {
		GenMsg = genmsg->next;
		free(genmsg);
		return OK;
	}

	while(genmsg != gm) {
		tmp = gm;
		NEXT(gm);
		if(gm == NULL)
			return ERROR;
	}

	tmp->next = gm->next;
	if(gm->body != NULL)
		textline_free(gm->body);
	free(gm);
	return OK;	
}

struct generated_message *
buffer_msg_to_user(char *name, char *buf)
{
	struct generated_message *tmp, *genmsg = GenMsg;

	while(genmsg) {
		if(!strcmp(genmsg->call, name))
			break;
		tmp = genmsg;
		NEXT(genmsg);
	}

	if(genmsg == NULL) {
		genmsg = malloc_struct(generated_message);
		strcpy(genmsg->call, name);
		
		if(GenMsg == NULL)
			GenMsg = genmsg;
		else
			tmp->next = genmsg;
		genmsg->next = NULL;
	}

	textline_append(&(genmsg->body), buf);
	return genmsg;
}

send_msg_to_user(char *name, char *subject, char *buf)
{
	struct msg_dir_entry msg, *m = &msg;
	struct generated_message *genmsg = buffer_msg_to_user(name, buf);
	int helplevel = MyHelpLevel;

	bzero(&msg, sizeof(msg));

	strcpy(msg.from.name.str, "BBS");
	strcpy(msg.to.name.str, name);

	msg.flags = MsgPersonal|MsgFrom;
	if(isCall(name))
		msg.flags |= MsgCall;
	else
		msg.flags |= MsgCategory;

	MyHelpLevel = 0;
	if(validate_send_request(m))
		return OK;
	MyHelpLevel = helplevel;

	sprintf(msg.bid, "$");
	msg.flags |= MsgBid;

	strncpy(m->sub, subject, 60);

	m->body = genmsg->body;
	genmsg->body = NULL;
	m->cdate = m->edate = Time(NULL);

	field_translation(m);
	if(msg_SendMessage(m) == ERROR)
		PRINTF("Error saving message\n");
	else
		system_msg_numstr(194, m->number, m->to.name.str);
	check_for_recpt_options(m);
	free_genmsg(genmsg);
	return OK;
}

