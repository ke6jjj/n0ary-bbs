#include <stdio.h>
#include <string.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tokens.h"
#include "function.h"
#include "user.h"
#include "message.h"
#include "vars.h"
#include "body.h"
#include "wp.h"
#include "rfc822.h"
#include "version.h"

int
	this_bbs_in_header;

/*==========================================================================
 * This routine is only called by the get_message_body() routine when a 
 * /CC command is supplied by the user. It reserves a new message directory
 * entry and prompts the user for a call/address.
 */

int
decode_cc_request(struct msg_dir_entry *m, char *str)
{
	struct msg_dir_entry new;

		/* Zero out all the fields that will be unique. This is required
		 * so that if the user doesn't specify contents for any field that
		 * we don't mistakenly apply old data.
		 */

	m->bid[0] = 0;
	m->to.at.str[0] = 0;
	m->to.name.str[0] = 0;
	m->flags &= ~(MsgToMask|MsgAtMask|MsgBid);

		/* run the call/address supplied by the user through our parser
		 * to get a new token list. Then translate those tokens into the
		 * message definition structure.
		 */

	parse_simple_string(str, SendOperands);
	msg_xlate_tokens(TokenList, &new);

		/* move relavent fields to the old structure. */

	m->flags |= (new.flags & (MsgToMask|MsgAtMask));
	bcopy(&new.to, &(m->to), sizeof(struct msg_address));
	m->flags &= ~MsgTypeMask;

		/* Do the WP lookup thing to make sure the new addressee is known
		 * to us.
		 */

	switch(m->flags & MsgToMask) {
	case MsgCategory:
	default:
		if(prefer_personal(m) == TRUE)
			m->flags |= MsgPersonal;
		else
			m->flags |= MsgBulletin;

		if(m->flags & MsgAtBbs) {
			if(determine_hloc_for_bbs(m))
				return ERROR;
		}
		break;

	case MsgCall:
		m->flags |= MsgPersonal;
		
		if(determine_homebbs(m))
			return ERROR;
		break;
	}

		/* if message is not being sent to a specific bbs then we need to
		 * add a bid.
		 */

	if(m->flags & (MsgBulletin | MsgAtDist)) {
		strcpy(m->bid, "$");
		m->flags |= MsgBid;
	}
	return OK;
}

void
msg_free_body(struct msg_dir_entry *m)
{
	m->body = textline_free(m->body);
}

/*========================================================================
 * open the necessary message body file and call the generic get_body
 * routine that is shared between file write and message send.
 */

int
get_message_body(struct msg_dir_entry *m)
{
	char buf[80];
	char orig_bbs[10];
	int maybe_dup = FALSE;
	int orig_num;

		/* tell user that we are now looking for the message body. If the
		 * help level is high enough show a little single line menu.
		 */

/* HELPMSG
!180
1000 Enter message body (/EXit, /ABort, /IN, /CC, /NU, /ED, /AD, /KI, /SI, /?):
0100 Msg (/EX /AB /IN /CC /NU /ED /AD /KI /SI /?):
0010 Msg:
*/
	system_msg(180);
	
		/* open up the file for the message body */

	orig_bbs[0] = 0;

		/* jump to the generic get_body routine */

	this_bbs_in_header = 0;

	switch(get_body(&(m->body), MSG_BODY, orig_bbs, &orig_num, &(m->edate))) {
	case mABORT:
		msg_free_body(m);
		return error(181);

	case mTERM:
		if(m->from.at.str[0] == 0)
			strcpy(m->from.at.str, orig_bbs);
		if(orig_bbs[0] == 0)
			m->flags |= MsgSentFromHere;
		if(orig_bbs[0] == 0)
			m->flags |= MsgSentFromHere;
		else {
			if(orig_num > 1) {
				char mid[80];
				sprintf(mid, "MID_%s_%d", orig_bbs, orig_num);
				if(mid_chk(mid))
					maybe_dup = TRUE;
				else
					bid_add(mid);
			}
		}

			/* up until now all messages that had a duplicate MID
			 * identified from the headers was simply held. This 
			 * routine should cause those messages to simply be
			 * deleted, actually dropped.
			 */

		if(maybe_dup == TRUE) {
			if(m->flags & MsgBulletin) {
				msg_free_body(m);
				return ERROR;
			}
		}

		field_translation(m);
		if(msg_SendMessage(m) == ERROR)
			PRINTF("Error saving message\n");
		else
			system_msg_numstr(194, m->number, m->to.name.str);
#if 1
		active_message = m->number;
#endif

		if(this_bbs_in_header >= Bbs_Msg_Loop)
			msg_hold_get_reason(m, "Loop detected");
		else 
			if(maybe_dup)
				msg_hold_get_reason(m, "Possible Duplicate?");

		msg_free_body(m);
		if(isCall(m->from.name.str) && orig_bbs[0] != 0) {
			wp_create_user(m->from.name.str);
			wp_set_field(m->from.name.str, wHOME, wGUESS, orig_bbs);
		}

		return OK;

	case mCC:
/* HELPMSG
!185
1000 CC ([CR] to quit):
0110 CC:

!188
1110    Unable to link message body files
1110 !! Aborting SEND command
*/
		do {
			if(orig_bbs[0] == 0)
				m->flags |= MsgSentFromHere;

			field_translation(m);
			if(msg_SendMessage(m) == ERROR)
				PRINTF("Error saving message\n");
			else
				system_msg_numstr(194, m->number, m->to.name.str);
#if 1
			active_message = m->number;
#endif

			check_for_recpt_options(m);

			system_msg(185);
			GETS(buf, 79);

			if(!isalpha(buf[0]))
				break;
			if(buf[0] == 0)
				break;

		} while(decode_cc_request(m, buf) == OK);

		msg_free_body(m);

		if(isCall(m->from.name.str) && orig_bbs[0] != 0) {
			wp_create_user(m->from.name.str);
			wp_set_field(m->from.name.str, wHOME, wGUESS, orig_bbs);
		}
		return OK;
	}

	return ERROR;
}

gen_vacation_body(struct text_line **tl, char *from)
{
	FILE *fpr;
	char fn[256];

	sprintf(fn, "%s/%s", Bbs_Vacation_Path, from);
	if((fpr = fopen(fn, "r")) == NULL)
		return ERROR;

	while(fgets(fn, 256, fpr) != NULL) {
		fn[strlen(fn)-1] = 0;
		textline_append(tl, fn);
	}

	fclose(fpr);
	return OK;
}

/*==========================================================================
 * Prompt the user for a subject to his message. If the user is not a bbs
 * and the subject is left blank, assume the user wanted to abort the
 * message.
 */

/* HELPMSG
!175
1000 Subject (60 characters max):
0100 Subject:
0010 Sj:
0001 OK
*/

int
get_subject(struct msg_dir_entry *m)
{
	char buf[4096];
	system_msg(175);
	GETS(buf, 4095);
	if(buf[0] == 0) {
		m->sub[0] = 0;

		if(ImBBS || (MyHelpLevel == 0))
			return OK;

		if(subject_not_required(m))
			return OK;

		return error(187);
	}

	buf[59] = 0;
	strcpy(m->sub, buf);
	return OK;
}

#define cSEPARATOR	':'
#define cTO			'>'
#define cFROM		'<'
#define cAT			'@'
#define cBID		'$'
#define cSUBJECT	'&'
#define cCOMMAND	'!'

int
subject_not_required(struct msg_dir_entry *m)
{
	char buf[80];
	FILE *fp = fopen(Bbs_NoSub_File, "r");
	int found = FALSE;
	long now = Time(NULL);

	if(fp == NULL) {
		char err[256];
		sprintf(err, "Could not open Bbs_NoSub_File [%s]", Bbs_NoSub_File);
		problem_report(Bbs_Call, BBS_VERSION, CmdLine, err);
		return FALSE;
	}

	while(fgets(buf, 80, fp)) {
		if(buf[0] == '\n' || buf[0] == ';' || isspace(buf[0]))
			continue;
		if(message_matches_criteria(buf, m, now)) {
			found = TRUE;
			break;
		}
	}

	fclose(fp);
	return found;
}

/*=========================================================================
 * This routine is called from the msg_body.c:get_body() function to copy
 * a message into another message. The inserted messages will be offset
 * from the left margin by "> " to make it easy to identify.
 */


int
include_msg(char *buf)
{
	struct msg_dir_entry *m;
	struct text_line *tl;
	char *p = buf;

		/* advance pointer to where the message number is */

	NextSpace(buf);
	NextChar(buf);
	
		/* if the message number starts with an alpha character then
		 * the user probably wanted a file to be included "/RF". In
		 * this case check that he is a sysop and and if so call
		 * the proper routines.
		 */

	if(isalpha(*buf) && ImSysop)
		return include_file(p);

		/* get the message number, grab the message directory entry and
		 * see if the user is allowed to veiw the message in question.
		 * if not abort.
		 */

/* HELPMSG
!184
1110 Message could not be included
*/

	if((m = msg_locate(get_number(&buf))) == NULL)
		return error(184);

	msg_ReadBody(m);

/* HELPMSG
!189
1100    Message #%N is not readable by you, you can therefore not include it.
0010 !! Not your message, include cancelled.
1100 !! INclude cancelled
*/
	set_user_list_info();
	if(set_listing_flags(m))
		return error_number(189, m->number);

		/* Ok, now open the message body file */

/* HELPMSG
!190
1110   Error %S message to include, request cancelled
*/
		/* First output the message header since it must be created from 
		 * the directory entry. It's not in the body.
		 * QUESTION: Should this be an abbreviated header to save space?
		 * It would make sense. By abbreviated I mean all info on a single
		 * line.
		 */

	link_line("\n");
	sprintf(buf, "> Msg# %5d  To: %s  Fr: %s  Subj: %s\n",
		m->number, m->to.name.str, m->from.name.str, m->sub);
	link_line(buf);
	link_line(">\n");

		/* now copy the message body to the new message */

	tl = m->body;
	while(tl) {
		if(strncmp(tl->s, "R:9", 3)) {
			sprintf(buf, "> %s\n", tl->s);
			link_line(buf);
			PRINTF("%s", buf);
		}
		NEXT(tl);
	}
	link_line("\n");
	PRINT("\n");
	return OK;
}

