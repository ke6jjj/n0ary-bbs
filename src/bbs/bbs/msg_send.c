#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "wp.h"
#include "vars.h"
#include "server.h"
#include "body.h"
#include "callbk.h"
#include "version.h"
#include "parse.h"
#include "msg_addr.h"
#include "msg_body.h"
#include "msg_mail.h"
#include "distrib.h"
#include "help.h"

extern int
	this_bbs_in_header;

static int
	in_distribution = FALSE,
	distribute_number = 0;

static char
	*not_accepted_reason = NULL;

int
msg_snd_t(struct TOKEN *head)
{
	struct msg_dir_entry msg;
	char logbuf[256];
	int result;

	bzero(&msg, sizeof(msg));

	if(head->token == MAIL) {
		PRINT("Send Internet function not operational\n");
		return OK;
	}

	if(head->token == DISTRIB) {
#if 0
		/* We used to call msg_xlate_tokens to parse the remainder of the
		 * SD command. Problem is that it would truncate the distribution
		 * list name to 6 characters. Now we should just check that the 
		 * next token is word or string and operate on it.
		 */
		if(msg_xlate_tokens(head, &msg) != OK)
			return ERROR;
#endif
		NEXT(head);
		if(head->token != WORD && head->token != STRING)
			return ERROR;
		return msg_snd_dist(&msg, head->lexem);
	}

	if(msg_xlate_tokens(head, &msg) != OK)
		return ERROR;
	result = msg_snd(&msg);

	if(not_accepted_reason != NULL)
		snprintf(logbuf, sizeof(logbuf), "\t%s\t%ld\t%s",
			not_accepted_reason, msg.number, CmdLine);
	else
		snprintf(logbuf, sizeof(logbuf), "\t%ld\t%s", msg.number, CmdLine);

	logd(logbuf);
	return result;
}

int
msg_rply_t(struct TOKEN *head)
{
	int num;

	if(head->token == NUMBER) {
		num= head->value;
		NEXT(head);
	} else
		if((num = active_message) == 0)
			return error(77);

	if(head->token != END)
		return error(192);

	msg_rply(num);
	return OK;
}

int
msg_copy_t(struct TOKEN *head)
{
	struct msg_dir_entry msg;
	int number;

	bzero(&msg, sizeof(msg));

	if(head->token == NUMBER) {
		number = head->value;
		NEXT(head);
	} else
		if((number = active_message) == 0)
			return error(77);

	if(msg_xlate_tokens(head, &msg) != OK)
		return ERROR;
	return msg_copy(number, &msg);
}

int
msg_copy_parse(int number, char *to)
{
	char cmd[80];
	sprintf(cmd, "%d %s", number, to);

	parse_simple_string(cmd, CallOperands);
	return msg_copy_t(TokenList);
}

int
msg_reject(struct msg_dir_entry *msg)
{
	FILE *fp = fopen(Bbs_Reject_File, "r");
	int reject = FALSE;
	long now = Time(NULL);
	char buf[80];

	if(fp == NULL)
		return FALSE;

	while(fgets(buf, 80, fp)) {
		if(buf[0] == '\n' || buf[0] == ';' || isspace(buf[0]))
			continue;
		if(message_matches_criteria(buf, msg, now)) {
			reject = TRUE;
			break;
		}
	}
	fclose(fp);
	return reject;
}

/* below this point there should be no reference to the original TOKEN list.
 */

int
msg_snd(struct msg_dir_entry *msg)
{
	char *hold = NULL;
	not_accepted_reason = NULL;

		/* check our immediate reject file */
	if(ImBBS)
		if(msg_reject(msg)) {
			PRINT("NO rejected\n");		
			not_accepted_reason = "REJECT";
			return ERROR;
		}

		/* first let's check for a unique bid */

	if(msg->flags & MsgBid)
		if(bid_chk(msg->bid)) {
			not_accepted_reason = "DUP";
			if(msg->flags & MsgAtDist)
				return error(174);

			if(msg->flags & (MsgPersonal|MsgNTS))
				hold = "Personal with a BID/MID, Duplicate";
			else
				return error(174);
		}
		

		/* if a from user was not specified then assume it the CurrentUser,
		 * only sysops and BBSs are allowed to fib about who they are.
		 */

	if(msg->flags & MsgFrom)
		if(!ImBBS && !ImSysop) {
			msg->flags &= ~MsgFrom;
			if(MyHelpLevel > 0)
				PRINT("*** Only bbss and sysops are allowed to lie about their call.\n");
		}

	if(!(msg->flags & MsgFrom)) {
		msg->flags |= MsgFrom;
		strcpy(msg->from.name.str, usercall);
	}

		/* now check to see if the type of message requested SP/ST/SB/SS
		 * matches what we find in the TO and AT fields. If not then the
		 * user may be informed of the error. User may be allowed to
		 * abort early.
		 *
		 * If CurrentUser is a bbs then don't bother.
		 */

	if(!ImBBS)
		if(validate_send_request(msg))
			return OK;

		/* if the message is @DISTRIBUTION rather than a homebbs and
		 * the user didn't supply a bid to use we need to apply a bid.
		 * This has to follow the reserve_msg_directory_entry() call,
		 * otherwise the message number doesn't exist.
		 */

	if((msg->flags & MsgAtDist || msg->flags & MsgBulletin) && !(msg->flags & (MsgBid|MsgNTS))) {
            /* If I am a bbs then this is an illegal bulletin. It should
			 * of had a bid when it entered here. So let's hold it.
			 */
		if(ImBBS)
			hold = "Incoming message should of had a bid!";
		else {
			strcpy(msg->bid, "$");
			msg->flags |= MsgBid;
		}
	}

		/* prompt the user for a subject. If the user is not a bbs and
		 * does not provide a subject then we assume he wishes to abort.
		 */

	if(get_subject(msg)) {
		not_accepted_reason = "NoSUBJ";
		if(!ImBBS)
			return OK;
	}

		/* Get the message body and handle the /CC command. This routine
		 * will take care of the overhead of aborting or saving the
		 * message and cleaning up the message directory.
		 */

	msg->cdate = msg->edate = Time(NULL);
	if(get_message_body(msg)) {
		not_accepted_reason = "ABORT";
		if(!ImBBS)
			return OK;
	}

	if(hold != NULL)
		msg_hold_get_reason(msg, hold);

	check_for_recpt_options(msg);

		/* We have successfully sent the message. Now if we are currently
		 * sending to a distribution list we have to let the caller know
		 * which message number to copy. This is a static variable in this
		 * module and we will set it even if we don't care.
		 */

	distribute_number = msg->number;

	return OK;
}

int
msg_snd_dist(struct msg_dir_entry *msg, char *fn)
{
	char call[80];
	int orig_help_level = MyHelpLevel;

	if(distrib_open(fn, call) == ERROR)
		return OK;

	PRINTF("Message to: %s\n", call);

	if(distrib_get_next(call) == ERROR) {
		PRINT("Distribution list is empty, notify the sysop of the problem.\n");
		return OK;
	}

	MyHelpLevel = 1;

		/* The first receipent in the distribution list will be the target of
		 * the original SEND.
		 */

	msg->flags &= ~(MsgToMask|MsgTypeMask|MsgAtMask);
	parse_simple_string(call, SendOperands);
	msg_xlate_tokens(TokenList, msg);

	distribute_number = 0;
	msg_snd(msg);

		/* If the distribute_number is non-zero then the send was
		 * successful and we can now do copies.
		 */

	if(distribute_number == 0) {
		distrib_close();
		MyHelpLevel = orig_help_level;
		return OK;
	}

	build_full_message_list();
	in_distribution = TRUE;
	while(distrib_get_next(call) == OK)
		msg_copy_parse(distribute_number, call);
	in_distribution = FALSE;
	MyHelpLevel = orig_help_level;
	return OK;
}

int
msg_rply(int num)
{
	struct msg_dir_entry *m, new;
	char buf[80];

		/* In a reply we need to grab the original message entry to allow
		 * us to copy some fields to the new message.
		 */

	if((m = msg_locate(num)) == NULL)
		return error_number(132, num);

		/* Copy fields from the original message to the new one */

	bzero(&new, sizeof(new));
	strcpy(new.to.name.str, m->from.name.str);
	strcpy(new.to.at.str, m->from.at.str);

			/* This takes the old subject and prepends "Re: " to it.
			 * if the message already has an "RE:" at the start we just
			 * add another ":" to give an indication of how many times
			 * it's been passed back and forth.
			 */

	case_strcpy(buf, m->sub, AllUpperCase);
	if(!strncmp(buf, "RE:", 3))
		sprintf(buf, "Re:%s", &(m->sub[3]));
	else
		sprintf(buf, "Re: %s", m->sub);
	buf[59] = 0;
	strcpy(new.sub, buf);

		/* Setup the fields that are not common. */

	SetMsgPersonal(&new);
	strcpy(new.from.name.str, usercall);

	new.cdate = new.edate = Time(NULL);

		/* Check for translations and bad distributions. This has to
		 * come before any forwarding or wp operations.
		 */

	field_translation(&new);

		/* Check for address of bbs, and readdress if necessary */

	if(determine_homebbs(&new)) {
		msg_free_body(&new);
		return ERROR;
	}
	

		/* This is a little strange but here goes. When a server function
		 * results in a message being generated to the originator then the
		 * server calls msg_rply() to do the addressing. Therefore it is
		 * necessary at this time to perform a callback to the server to
		 * provide us with a body. If we were not called by a server then
		 * we call get_message_body() just like sending a message.
		 */

	switch(InServer) {
	case 'V':
		strcpy(new.from.name.str, m->to.name.str);
		if(gen_vacation_body(&new.body, new.from.name.str) == ERROR)
			break;
		field_translation(&new);
		msg_SendMessage(&new);
		msg_free_body(&new);
		break;

	case 'C':
		gen_callbk_body(&new.body);
		field_translation(&new);
		msg_SendMessage(&new);
		msg_free_body(&new);
		break;
	case 'W':
	case 'F':
		break;
	default:
			/* This routine will get the message body and handle the /cc
			 * command if present. It will also update the message directory
			 * if the message is saved or aborted.
			 */
		if(get_message_body(&new))
			return OK;
	}
	check_for_recpt_options(&new);
	return OK;
}

int
msg_copy(int num, struct msg_dir_entry *msg)
{
	struct msg_dir_entry *m, new;

		/* start by getting the message directory entry of the message
		 * we wish to copy.
		 */

	if((m = msg_locate(num)) == NULL)
		return error_number(132, num);

		/* Use the msg_list function set_listing_flags() to determine if the
		 * requested message is visible to us. Otherwise a user could simply
		 * copy another persons message to himself then have access to it.
		 *
		 * The one exception is a bbs. BBS's send messages under someone elses
		 * callsign, meaning that they are writing messages that will be
		 * invisible to them if they try to list/read or copy.
		 */

	if(!ImBBS) {
		set_user_list_info();
		if(set_listing_flags(m))
			return error_number(191, m->number);
	}

		/* copy the message entry to a new static copy. We don't want
		 * to trash our live image.
		 */

	bcopy(m, &new, sizeof(new));
	bzero(&new.entries, sizeof(new.entries));
	new.header = new.read_by = NULL;

		/* Now change the fields that should be changed, basically just
		 * who sent the message. The person that requested the copy op
		 * will now look like the originator. This is fairly confusing
		 * when using COPY to forward a message. But that's life,
		 * otherwise the receipent will assume that the message was
		 * directed at them personally with no knowledge of who the
		 * original receipent was.
		 */

	strcpy(new.from.name.str, usercall);
	new.from.at.str[0] = 0;

	    /* Kill the bid, we don't want to create a duplicate */

	new.bid[0] = 0;

		/* Blast the read history for the message and reinitialize the
		 * message flags.
		 */

	new.read_cnt = 0;
	new.flags &= MsgWriteMask;
	new.flags &= ~(MsgPending|MsgRead|MsgBid);
	new.flags |= MsgSentFromHere;
	new.cdate = new.edate = Time(NULL);
	new.kdate = 0;

		/* set message number to the source and read the body. Then
		 * clear the message number.
		 */
	new.number = num;
	msg_ReadBody(&new);
	new.number = 0;

		/* now strip off the leading R: lines */
	{
		struct text_line *tl;
		while(new.body) {
			if(strncmp(new.body->s, "R:", 2))
				break;
			tl = new.body;
			new.body = tl->next;
			free(tl);
		}
	}

	if(!in_distribution) {
		char buf[256];
		if(m->to.at.str[0])
			snprintf(buf, sizeof(buf), "[Copy Of: Msg# %5ld  To: %s@%s  Fr: %s]",
				m->number, m->to.name.str, m->to.at.str, m->from.name.str);
		else
			snprintf(buf, sizeof(buf), "[Copy Of: Msg# %5ld  To: %s  Fr: %s]",
				m->number, m->to.name.str, m->from.name.str);
		textline_append(&(new.body), "");
		textline_append(&(new.body), buf);
	}

		/* now merge in the info from our command line*/

	new.flags &= ~(MsgTypeMask|MsgToMask|MsgAtMask|MsgHloc);

	new.flags |= (msg->flags & MsgTypeMask);
	if(msg->flags & MsgToMask) {
		new.flags |= (msg->flags & MsgToMask);
		strcpy(new.to.name.str, msg->to.name.str);
	}
	if(msg->flags & MsgAtMask) {
		new.flags |= (msg->flags & MsgAtMask);
		strcpy(new.to.at.str, msg->to.at.str);
	}
	if(msg->flags & MsgHloc) {
		new.flags |= MsgHloc;
		strcpy(new.to.address, msg->to.address);
	}

		/* is message is secure then we want to skip any further
		 * checks.
		 */

	if(m->flags & MsgSecure) {
		new.flags &= ~(MsgTypeMask|MsgAtMask|MsgHloc);
		new.flags |= MsgSecure;
		new.to.at.str[0] = 0;
		new.to.address[0] = 0;
		PRINT("Secure messages must stay secure\n");
	} else {

		/* the message entry is now built. We need to do the standard
		 * address and WP checks to make sure the new receipent is
		 * addressed properly.
		 */

		switch(new.flags & MsgToMask) {
		case MsgCall:
			if(determine_homebbs(&new)) {
				msg_free_body(&new);
				return ERROR;
			}
			SetMsgPersonal(&new);
			break;

		case MsgCategory:
		default:
			if(new.flags & MsgAtBbs)
				if(determine_hloc_for_bbs(&new)) {
					msg_free_body(&new);
					return ERROR;
				}
			new.flags |= MsgBid;
			strcpy(new.bid, "$");
			SetMsgBulletin(&new);
			break;

		case MsgToNts:
			determine_nts_address(&new);
			SetMsgNTS(&new);
			break;
		}
	}
		/* Finish up by setting up for forwarding and check for translation
		 * changes.
		 *
		 * QUESTION: The order here may need to be worked on, I beleive
		 * it may be a problem having set_forwarding() prior to
		 * field_translation() but it seems to be working now.
		 */

	field_translation(&new);
	if(msg_SendMessage(&new) == ERROR)
		PRINTF("Error saving message\n");
	else
		system_msg_numstr(194, new.number, new.to.name.str);
	msg_free_body(&new);

/* HELPMSG
!194
1110 Message #%N to %S stored
*/

	check_for_recpt_options(&new);

	return OK;
}

void
check_for_recpt_options(struct msg_dir_entry *m)
{
		/* Only our users can have this option selected, it's a quick
		 * test.
		 */

	build_full_message_list();
	if(user_focus(m->to.name.str) == ERROR) {
		user_focus(usercall);
		return;
	}

/* This routine will check to see if the addressee is one of our users and
 * if EMAIL forwarding is requested. If so the message is feed to the mail
 * system and the original message left on the bbs.
 *
 * QUESTION: Do we really want to leave the message behind or delete it?
 * We should collect thoughts on this or possibly make it a user option.
 * We may also wish to label it as old, speeding it's aging.
 */

	if(AutoEMAILForward) {
		if(user_check_flag(uEMAILALL) || !strcmp(m->to.at.str, Bbs_Call)) {
			if(msg_mail(m->number, user_get_field(uADDRESS)) == OK) {
				msgd_cmd_num(msgd_xlate(mKILL), m->number);
			} else {
				printf("*** DEBUG ***\n** msg_mail(): failed\n");
		printf("** probably the 'timeout occured' message from the smtp_msg_send call.\n");
				printf("**   msg number: %ld\n", m->number);
				printf("**    addressee: %s\n", m->to.name.str);
				printf("**         user: %s\n", usercall);
				printf("**           To: %s\n", user_get_field(uADDRESS));
				printf("**      version: %s\n", BBS_VERSION);
				printf("*************\n");
			}
		}
	}

		/* Check if user is on vacation. If so then keep the message
		 * around longer.
		 */

	if(AutoVacation) {
		int orig_help_level = MyHelpLevel;
		MyHelpLevel = 1;

		InServer = 'V';
		msg_rply(m->number);
		InServer = FALSE;
		msgd_cmd_num(msgd_xlate(mIMMUNE), m->number);
		MyHelpLevel = orig_help_level;
	}

		/* one last thing, we would like to touch the users time
		 * stamp so that messages going either way through the bbs
		 * will show activity in the account.
		 */

#if 0
	touch_user(m->to.name.str, 0);
	touch_user(m->from.name.str, 0);
#endif

	user_focus(usercall);
}

static char *
get_route_addr(char **str)
{
	static char buf[20];
	char *p = buf;
	int cnt = 0;
	buf[0] = 0;

	if(**str) {
		while(isalnum(**str)) {
			*p++ = *(*str)++;
			if(++cnt > 8) {
				NextSpace(*str);
				break;
			}
		}
		*p = 0;
		NextChar(*str);
	} else
		return NULL;
	return buf;
}	

/*======================================================================
 * Read the routing for this message. As we pass each bbs update it's
 * entry in the wp. Assume it to be WP_UserIndirect which will inhibit
 * the change bit being set.
 *
 * Entry looks like this, the R: was eliminated by the caller
 *
 *   R:910823/2118 @:N0ARY.#NOCAL.CA.USA.NA Sunnyvale, CA #:3849 Z:94086
 *   R:910823/2118 3849@N0ARY.#NOCAL.CA.USA.NA Sunnyvale, CA Z:94086
 */

void
read_routing(char *buf, char *homebbs, time_t *orig_date, int *num)
{
	char *p, hloc[LenHLOC];
	struct tm ltm, *tm;

	tm = gmtime_r(orig_date, &ltm);

	/* ignore the supplied date for now. */
	NextChar(buf);
	strptime(buf, "%y%m%d/%H%M", tm);
	tm->tm_sec = 0;
#ifdef HAVE_TIMEGM
	*orig_date = timegm(tm);
#else
	*orig_date = mktime(tm); /* was timelocal -- rwp */
#endif

	NextSpace(buf);
	NextChar(buf);

	*num = ERROR;
	if(isdigit(*buf))
		*num = get_number(&buf);

			/* find the '@' which identifies the BBS entry */

	if((p = (char*)index(buf, '@')) == NULL) {
		*homebbs = 0;
		return;
	}

			/* go past ':' if it exists and white to the callsign */

	while(!isalnum(*p))
		p++;

			/* grab call N0ARY.#NOCAL...
			 *                ^-- stops at period
			 * this also assumes that the trustee of the bbs uses it as
			 * his home.
			 */
	case_strcpy(homebbs, get_route_addr(&p), AllUpperCase);
	if(!strcmp(homebbs, Bbs_Call))
		this_bbs_in_header++;

			/* now parse out the remainder of the address
			 */

	p++;

	if(*p && *p != '\n') {
		if(IsPrintable(homebbs) && (strlen(homebbs) > 3)) {
			strncpy(hloc, get_string(&p), LenHLOC);
			wp_create_bbs(homebbs);
			wp_set_field(homebbs, wHOME, wUSER, homebbs);
			if(IsPrintable(hloc) && (strlen(hloc) > 3))
				wp_set_field(homebbs, wHLOC, wUSER, hloc);
		}
	}

	if(*num == ERROR) {
		*num = 0;
		while(*p) {
			if((p = (char*)index(p, '#')) == NULL)
				break;
			p++;
			if(*p == ':') {
				p++;
				*num = atoi(p);
				break;
			}
		}
	}
}
