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
#include "wp.h"
#include "vars.h"
#include "version.h"
#include "msg_addr.h"

#define History(x)		history |= x
#define First_Time(x)	!(history &= x)
#define TO_NTS			1
#define P_GROUP			2
#define P_ATDIST		4
#define B_PREFER		8
#define B_TO_CALL		0x10
#define B_TO_GROUP		0x20
#define T_TO_CALL		0x40
#define T_TO_GROUP		0x80
#define S_AT			0x100

/* Typically messages to GROUPS/CATEGORIES that are of type PERSONAL
 * are misaddressed. This is because a personal message to SALE can
 * only be read by sysops. The bbs has a file "Bbs_Personal_File" that
 * defines the messages that are allowed to be sent personal to a
 * catagory. As a matter of fact it is advised. This refers to messages
 * to servers, REQFIL, REQDIR, etc. and message addressed to SYSOP.
 */

static int
	validate_message(struct msg_dir_entry *msg, int history);

int
prefer_personal(struct msg_dir_entry *msg)
{
	FILE *fp = fopen(Bbs_Personal_File, "r");
	int found = FALSE;
	long now = Time(NULL);
	char buf[80];

		/* if the file doesn't exist make the user confirm the choice */

	if(fp == NULL)
		return FALSE;

	while(fgets(buf, 80, fp)) {
		if(buf[0] == '\n' || buf[0] == ';' || isspace(buf[0]))
			continue;
		if(message_matches_criteria(buf, msg, now)) {
			found = TRUE;
			break;
		}
	}
	fclose(fp);
	return found;
}

/* Pass all NTS messages through here to have there addresses checked for
 * conformance. If the address doesn't match should we force PERSONAL
 * instead? For now just abort the send?
 *
 * The only two valid forms at this point are:
 *    99999 @ NTSst
 *    NTSst @ NTSst
 */

/*ARGSUSED*/
int
determine_nts_address(struct msg_dir_entry *msg)
{
	return OK;
}

/* Use the HlocScript to lead the user through a series of questions
 * to build a good hloc. We don't have one for this bbs.
 */
static int
prompt_user_for_hloc(struct msg_dir_entry *msg)
{
	FILE *fp;
	int done = FALSE;
	char buf[80];

	msg->flags &= ~MsgHloc;
	if((fp = fopen(Bbs_Hloc_Script, "r")) == NULL)
		return OK;

	PRINTF("\n** The location of the %s is not known to this system.\n", msg->to.at.str);
	PRINTF("** You will have to supply additional information to route the\n");
	PRINTF("** message successfully.\n");

	while(!done && fgets(buf, 80, fp)) {
		switch(buf[0]) {
		case '?':
			buf[strlen(buf)-1] = 0;
			PRINTF("\n%s (Y/n/q)? ", &buf[1]);
			switch(get_yes_no_quit(YES)) {
			case YES:
				while(fgets(buf, 80, fp)) {
					if(buf[0] == '?' || buf[0] == '!') {
						done = TRUE;
						break;
					}
					if(buf[0] != '#')
						PRINTF("%s", buf);
				}
			case NO:
				break;

			case QUIT:
				fclose(fp);
				return ERROR;
			}
			break;

		case '!':
			while(fgets(buf, 80, fp))
				PRINTF("%s", buf);
			done = TRUE;
		}
	}

	fclose(fp);
	GETS(buf, LenHLOC);
	case_string(buf, AllUpperCase);
	strncpy(msg->to.address, buf, LenHLOC);
	msg->flags |= MsgHloc;
	return OK;
}


/* At this point we have a valid TO and AT field but are unsure
 * about the HLOC. It could be there, missing, or wrong.
 */
int
determine_hloc_for_bbs(struct msg_dir_entry *msg)
{
	char hloc[256];

		/* check to see if BBS and subsequently HLOC are in the WP.
		 * If not set hloc equal to "?" so we can identify it.
		 */

	hloc[0] = '?';
	if(wp_cmd(msg->to.at.str) == OK) {
		strcpy(hloc, wp_get_field(msg->to.at.str, wHLOC));
		if(!strncmp(hloc, "NO,", 3))
			hloc[0] = '?';
	}

		/* if the HLOC is not in the WP but the user has already
		 * supplied a HLOC use that. If the user has not supplied
		 * an HLOC then prompt him for one.
		 */

	if(hloc[0] == '?') {
		if(msg->flags & MsgHloc)
			return OK;
		return prompt_user_for_hloc(msg);
	}

		/* we did get an hloc and the user neglected to supply one so
		 * let's use ours.
		 */

	if(!(msg->flags & MsgHloc)) {
		msg->flags |= MsgHloc;
		strcpy(msg->to.address, hloc);
		return OK;
	}

		/* We have an HLOC in the WP and the user has supplied an
		 * hloc. They could be the same or different. If they are
		 * the same to the length of the user supplied version then
		 * use ours. The user will often not fully qualify.
		 *
		 *   user: #NOCAL.CA
		 *     wp: #NOCAL.CA.USA.NA
		 *
		 * If it is not an exact full length match then tell the user
		 * what you are doing if HELP level is 2 or 3.
		 */
	
	if(!strncmp(msg->to.address, hloc, strlen(msg->to.address))) {
		if(MyHelpLevel > 1 && strcmp(msg->to.address, hloc))
			PRINTF("Completing your HLOC entry: %s.%s\n",
				msg->to.at.str, hloc);
		strcpy(msg->to.address, hloc);
		return OK;
	}

		/* The HLOCs are definitely different. We need to prompt the
		 * user to choose which on to use.
		 */

	if(MyHelpLevel > 1) {
		PRINTF("** The HLOC you supplied is different than the one in our\n");
		PRINTF("** local white pages.\n");
		if(MyHelpLevel > 2) {
			PRINTF("**    yours: %s\n", msg->to.address);
			PRINTF("**     mine: %s\n", hloc);
			PRINTF("Do you wish to override my default selection (y/N/q)? ");
			switch(get_yes_no_quit(NO)) {
			case NO:
				strncpy(msg->to.address, hloc, LenHLOC);
				break;
			case QUIT:
				return ERROR;
			}
		}
	}
	return OK;
}

/* given a callsign in the TO field we need to determine the homebbs
 * or prompt the user for it.
 */

int
determine_homebbs(struct msg_dir_entry *msg)
{
	char home[256];

		/* check to see if USER and subsequently HOME are in the WP.
		 * If not set home equal to "?" so we can identify it.
		 */

	home[0] = '?';
	if(wp_cmd(msg->to.name.str) == OK) {
		strcpy(home, wp_get_field(msg->to.name.str, wHOME));
		if(!strncmp(home, "NO,", 3))
			home[0] = '?';
	}

		/* the HOME is not in the WP. If the user went ahead and
		 * entered one then lets use it. If not then we need to ask
		 * him for one.
		 */

	if(home[0] == '?') {
		if(!(msg->flags & MsgAtBbs)) {
			PRINTF("** The user %s is not in the local WP. We need the homebbs\n",
				msg->to.name.str);
			PRINT("** to be able to route your message.\n");
			PRINTF("Please enter a homebbs [%s]? ", Bbs_Call);
			if(NeedsNewline)
				PRINT("\n");
			if (GETnSTRdef(msg->to.at.str, 80, AllUpperCase, Bbs_Call) == NULL)
	return ERROR;
			msg->flags |= MsgAtBbs;
		}
		return determine_hloc_for_bbs(msg);
	}

		/* if the user didn't supply a home and we have one now then use it */

	if(!(msg->flags & MsgAtMask)) {
		int result;
		msg->flags |= MsgAtBbs;
		strcpy(msg->to.at.str, home);
		result = determine_hloc_for_bbs(msg);
		if(result == OK && MyHelpLevel > 1)
			PRINTF("Home BBS: %s.%s\n", msg->to.at.str, msg->to.address);
		return result;
	}

		/* The user supplied a homebbs and we have one as well. Now
		 * let's see if we agree. If so move on down the road and 
		 * determine the hloc.
		 */

	if(!strcmp(msg->to.at.str, home))
		return determine_hloc_for_bbs(msg);

		/* If we differ than we need to ask the user who is right.
		 */

	if(MyHelpLevel > 1) {
		PRINTF("** The HOMEBBS you supplied, %s, is different than the\n", msg->to.at.str);
		PRINTF("** one in our local white pages, %s.\n", home);
		if(MyHelpLevel > 2) {
			PRINTF("Do you wish to override my default selection (y/N/q)? ");
			switch(get_yes_no_quit(NO)) {
			case NO:	/* Use HOME from white pages */
				strcpy(msg->to.at.str, home);
				msg->flags |= MsgAtBbs;
				return determine_hloc_for_bbs(msg);
			case YES:	/* Use HOME supplied by user, ignore WP */
				return determine_hloc_for_bbs(msg);
			case QUIT:
				return ERROR;
			}
		}
	}
	return determine_hloc_for_bbs(msg);
}

/*=========================================================================
 * check_personal(struct msg_dir_entry *)
 * check_bulletin(struct msg_dir_entry *)
 * check_secure(struct msg_dir_entry *)
 * check_nts(struct msg_dir_entry *)
 *
 * These routines are called to check to see if the message type requested
 * by the sender is appropriate with respect to the TO and AT fields.
 * The user is told of problems and prompted for confirmation if the help
 * level is high enough.
 *
 * All of these routines will return in one of three ways:
 *
 *  OK -	The message definition was not altered in the course of the
 *			procedure.
 *
 *  TRUE -	The message definition was altered and the structure should
 *			be passed through the check_xxxxx() procedure again.
 *
 *  ERROR -	Abort the message
 */

int
check_personal(struct msg_dir_entry *msg, int history)
{
	char buf[256];

		/* attempt to remove the SP GROUP stuff upfront. */

	if(msg->flags & MsgCategory && !prefer_personal(msg)) {
		if(MyHelpLevel > 1 && First_Time(P_GROUP)) {
			History(P_GROUP);
			PRINTF("** This is a PERSONAL message being sent to a CATEGORY which\n");
			PRINTF("** means it will only be readable by sysops and servers.\n");
			if(MyHelpLevel > 2) {
				PRINTF("Do you wish to change it to a BULLETIN (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->flags &= ~MsgTypeMask;
					msg->flags |= MsgBulletin;
					return validate_message(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}
	}

	switch(msg->flags & (MsgToMask | MsgAtMask)) {

	case MsgCall|MsgAtBbs:		/* N6ZFJ@N0ARY... */
	case MsgCall:				/* N6ZFJ */
		return determine_homebbs(msg);

	case MsgCall|MsgAtDist:		/* N6ZFJ@ALLCA... */
		if(MyHelpLevel > 1 && First_Time(P_ATDIST)) {
			History(P_ATDIST);
			PRINTF("** This looks like a message to an individual but you have not\n");
			PRINTF("** specified a homebbs but instead a distribution, @%s.\n", msg->to.at.str);
			if(MyHelpLevel > 2) {
				PRINTF("Do you want to attempt delivery to a bbs (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->to.at.str[0] = 0;
					msg->flags &= ~MsgAtMask;
					return check_personal(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}
		return OK;

			/* if this were a bulletin we would probably want to inform the user
			 * that this message is not going to forward. But since it is a
			 * personal, we are probably ok. */
	case MsgCategory:				/* SALE	*/
	case MsgCategory|MsgAtBbs:		/* SALE@N0ARY... */
		return OK;

	case MsgCategory|MsgAtDist:		/* SALE@ALLCA... */
		return OK;

				/* this has already been checked in validate_message() */	

	case MsgToNts|MsgAtBbs:			/* 94086@N0ARY or NTSCA@N0ARY */
	case MsgToNts|MsgAtDist:		/* 94086@ALLCA or NTSCA@ALLCA */
	case MsgToNts:				/* 94086 or NTSCA */
		return OK;

	default:
		sprintf(buf, "Invalid TO/TYPE [0x%lx]", msg->flags);
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__, "check_personal", CmdLine, buf);
	}
	return OK;
}

static int
check_bulletin(struct msg_dir_entry *msg, int history)
{
	char buf[256];

		/* Some messages that look like BULLETINs should actually be PERSONAL.
		 * Messages like SP SYSOP or SP IPGATE.
		 */

	if(msg->flags & MsgCategory && prefer_personal(msg)) {
		if(MyHelpLevel > 1 && First_Time(B_PREFER)) {
			History(B_PREFER);
			PRINTF("** This message is being sent as a BULLETIN. The addressee, %s,\n",
				msg->to.name.str);
			PRINTF("** normally expects PERSONAL type messages.\n");
			if(MyHelpLevel > 2) {
				PRINTF("Do you wish to change it to a PERSONAL (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->flags &= ~MsgTypeMask;
					msg->flags |= MsgPersonal;
					return validate_message(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}
	}

	switch(msg->flags & (MsgToMask | MsgAtMask)) {

	case MsgCall|MsgAtBbs:		/* N6ZFJ@N0ARY... */
	case MsgCall:				/* N6ZFJ */
	case MsgCall|MsgAtDist:		/* N6ZFJ@ALLCA... */
		if(MyHelpLevel > 1 && First_Time(B_TO_CALL)) {
			History(B_TO_CALL);
			PRINTF("** This looks like a message to an individual and therefore should\n");
			PRINTF("** be sent as a PERSONAL message as opposed to a BULLETIN.\n");
			if(MyHelpLevel > 2) {
				PRINTF("Do you wish to change it to a PERSONAL (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->flags &= ~MsgTypeMask;
					msg->flags |= MsgPersonal;
					return validate_message(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}
		return OK;

	case MsgCategory:				/* SALE	*/
		if(MyHelpLevel > 1 && First_Time(B_TO_GROUP)) {
			History(B_TO_GROUP);
			PRINTF("** This bulletin will not forward off the bbs.\n");
			if(MyHelpLevel > 2) {
				PRINTF("** You must specify a desired area to flood if you want the message\n");
				PRINTF("** to be sent to other bbss. Type INFO BULLETIN at the main prompt\n");
				PRINTF("** for more information.\n");
				PRINTF("Do you wish to abort the message (y/N)? ");
				if(get_yes_no(NO) == YES)
					return ERROR;
			}
		}
		return OK;

	case MsgCategory|MsgAtBbs:		/* SALE@N0ARY... */
		return OK;

	case MsgCategory|MsgAtDist:		/* SALE@ALLCA... */
		return OK;

			
				/* this has already been verified on entry to validate_message() */

	case MsgToNts|MsgAtBbs:			/* 94086@N0ARY or NTSCA@N0ARY */
	case MsgToNts|MsgAtDist:		/* 94086@ALLCA or NTSCA@ALLCA */
	case MsgToNts:				/* 94086 or NTSCA */
		return OK;

	default:
		sprintf(buf, "Invalid TO/TYPE [0x%lx]", msg->flags);
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__, 
				   "check_bulletin", CmdLine, buf);
	}
	return OK;
}

static int
check_nts(struct msg_dir_entry *msg, int history)
{
	char buf[256];

	switch(msg->flags & (MsgToMask | MsgAtMask)) {

	case MsgCall|MsgAtBbs:		/* N6ZFJ@N0ARY... */
	case MsgCall:				/* N6ZFJ */
	case MsgCall|MsgAtDist:		/* N6ZFJ@ALLCA... */
		if(MyHelpLevel > 1 && First_Time(T_TO_CALL)) {
			History(T_TO_CALL);
			PRINTF("** This looks like a message to an individual and therefore should\n");
			PRINTF("** be sent as a PERSONAL message as opposed to an NTS. NTS messages\n");
			PRINTF("** require great care in addressing, this doesn't look like proper.\n");
			if(MyHelpLevel > 2) {
				PRINTF("Do you wish to change it to a PERSONAL (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->flags &= ~MsgTypeMask;
					msg->flags |= MsgPersonal;
					return validate_message(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}
		return OK;

	case MsgCategory:				/* SALE	*/
	case MsgCategory|MsgAtBbs:		/* SALE@N0ARY... */
	case MsgCategory|MsgAtDist:		/* SALE@ALLCA... */
		if(MyHelpLevel > 1 && First_Time(T_TO_GROUP)) {
			History(T_TO_GROUP);
			PRINTF("** This looks like a message to a catagory and therefore should\n");
			PRINTF("** be sent as a BULLETIN as opposed to an NTS. NTS messages\n");
			PRINTF("** require great care in addressing, this doesn't look like proper.\n");
			if(MyHelpLevel > 2) {
				PRINTF("Do you wish to change it to a BULLETIN (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->flags &= ~MsgTypeMask;
					msg->flags |= MsgBulletin;
					return validate_message(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}
		return OK;
			
				/* this has already been verified on entry to validate_message() */

	case MsgToNts|MsgAtBbs:			/* 94086@N0ARY or NTSCA@N0ARY */
	case MsgToNts|MsgAtDist:		/* 94086@ALLCA or NTSCA@ALLCA */
	case MsgToNts:				/* 94086 or NTSCA */
		return OK;

	default:
		sprintf(buf, "Invalid TO/TYPE [0x%lx]", msg->flags);
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__, 
				   "check_nts", CmdLine, buf);
	}
	return OK;
}

static int
check_secure(struct msg_dir_entry *msg, int history)
{
	switch(msg->flags & MsgAtMask) {
	case MsgAtBbs:
	case MsgAtDist:
		if(!strcmp(msg->to.at.str, Bbs_Call))
			break;

		if(MyHelpLevel > 1 && First_Time(S_AT)) {
			History(S_AT);
			PRINTF("** Secure messages are not allowed to forward over the radio and\n");
			PRINTF("** are therefore trapped on this bbs. You have specified a BBS or\n");
			PRINTF("** distribution for this message, it is being removed.\n");
		}
		msg->to.at.str[0] = 0;
		msg->flags &= ~MsgAtMask;
		return OK;
	}
	return OK;
}

/* This routine is called recursively until the message TO/AT/HLOC/TYPE
 * fields make sense. This is to identify things like PERSONAL to a CATEGORY
 * or a BULLETIN to an individual.
 */

static int
validate_message(struct msg_dir_entry *msg, int history)
{
	char buf[256];

	if(msg->flags & MsgToNts && !(msg->flags & MsgNTS) && First_Time(TO_NTS))
		if(MyHelpLevel > 1) {
			History(TO_NTS);
			PRINTF("** This looks like an NTS message by the address. NTS messages\n");
			PRINTF("** should be sent with the SEND NTS (ST) command and the\n");
			PRINTF("** addressing is very important.\n");
			if(MyHelpLevel > 2) {
				PRINTF("Do you want to change this to an NTS message (Y/n/q)? ");
				switch(get_yes_no_quit(YES)) {
				case YES:
					msg->flags &= ~MsgTypeMask;
					msg->flags |= MsgNTS;
					return validate_message(msg, history);
				case QUIT:
					return ERROR;
				}
			}
		}

	switch(msg->flags & MsgTypeMask) {
	case MsgPersonal:
		return check_personal(msg, history);
	case MsgBulletin:
		return check_bulletin(msg, history);
	case MsgNTS:
		return check_nts(msg, history);
	case MsgSecure:
		return check_secure(msg, history);
	}

	sprintf(buf, "Invalid TYPE [0x%lx]", msg->flags);
	bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__, 
			   "validate_message", CmdLine, buf);
	return ERROR;
}

int
validate_send_request(struct msg_dir_entry *msg)
{
	int history = 0;
		/* those identified as non-hams are only allowed to send SECURE
		 * messages. We cannot allow their messages to be sent over RF.
		 * If it was sent as secure just let it go.
		 */

	if(ImSuspect || ImNonHam) {
		msg->flags &= ~MsgTypeMask;
		msg->flags |= MsgSecure;
		if(msg->flags & MsgSecure)
			return OK;

PRINT("The bbs has identified you as a non-ham. If this is incorrect please\n");
PRINT("send a message to the SYSOP. As a non-ham your mail is not allowed to\n");
PRINT("forward over the radio. To keep this from occuring your message has been\n");
PRINT("changed to SECURE. This means it is only accessible via the non-radio\n");
PRINT("ports.\n\n");
		return OK;
	}

		/* If the message was sent without specifying a TYPE than we need to
		 * try and determine what type to apply to this message.
		 */

	if(!(msg->flags & MsgTypeMask))
		switch(msg->flags & MsgToMask) {
		case MsgCategory:
			if(prefer_personal(msg))
				msg->flags |= MsgPersonal;
			else
				msg->flags |= MsgBulletin;
			break;

		case MsgCall:
			msg->flags |= MsgPersonal;
			break;

		case MsgToNts:
			msg->flags |= MsgNTS;
			break;
	
		default:
			PRINT("validate_send_request: bad MsgToMask\n");
			return ERROR;
		}

	return validate_message(msg, history);
}

