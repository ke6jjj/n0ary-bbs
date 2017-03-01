#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "vars.h"
#include "pending.h"
#include "msg_util.h"

static int execute_replacement(char *replace, struct msg_dir_entry *m);

#if 0

 Translation file. This file is run in a recursive fashion so care should
 be taken when inserting fields.

   Language:
       token match : token replacement

   Match Tokens:
       < from
       > to
       @ distribution
       $ bid (regular expression)

   Replacement Tokens:
       < from
       > to
       @ distribution
       $ bid (regular expression)
       ! command [args]

   Variables that can be carried from match to replacement:
       [to]
       [from]
       [subject]
       [bid]
       [at]
#endif

extern int
	debug_level;

static char *
macro_replace(char *str, struct msg_dir_entry *m)
{
	static char buf[256];
	char *p = buf;
	char *token;

	while(*str) {
		if(*str != '[') {
			*p++ = *str++;
			continue;
		}

			/* macro found, pull it out */

		str++;
		token = str;

		if((str = (char*)index(str, ']')) == NULL) {
			PRINT("Badly formed macro, no ending ']'\n");
			exit(1);
		}
		*str++ = 0;

		case_string(token, AllUpperCase);
		if(!strcmp(token, "SUBJECT")) {
			token = m->sub;
			while(*token && *token != ' ')
				*p++ = *token++;
			continue;
		}

		if(!strcmp(token, "TO")) {
			token = m->to.name.str;
			while(*token)
				*p++ = *token++;
			continue;
		}

		if(!strcmp(token, "FROM")) {
			token = m->from.name.str;
			while(*token)
				*p++ = *token++;
			continue;
		}

		if(!strcmp(token, "BID")) {
			token = m->bid;
			while(*token)
				*p++ = *token++;
			continue;
		}

		if(!strcmp(token, "AT")) {
			token = m->to.at.str;
			while(*token && *token != ' ' && *token != '.')
				*p++ = *token++;
			continue;
		}
	}

	*p = 0;
	return buf;
}
void
field_translation(struct msg_dir_entry *m)
{
	FILE *fp = fopen(Bbs_Translate_File, "r");
	char buf[256];
	time_t now = Time(NULL);

	if(fp == NULL)
		return;

	while(fgets(buf, 256, fp)) {
		char 
			*match = buf,
			*replace;

			/* if line is a comment or blank skip */

		NextChar(match);
		if(*match == '\n' || *match == ';')
			continue;

			/* a line is made up of the following:
			 *		match_tokens separator replacement_tokens
			 * if we can't find the seperator then skip the line.
			 */

		if((replace = (char*) index(buf, cSEPARATOR)) == NULL)
			continue;

			/* NULL terminate the match string */

		*replace++ = 0;
		NextChar(replace);

		if(message_matches_criteria(match, m, now) == TRUE) {
			if(debug_level & DBG_MSGTRANS)
				PRINTF("TRAN ident: %s => %s", match, replace);
			if(execute_replacement(replace, m) == TRUE)
				break;
		}
	}

	fclose(fp);
}


static int
execute_replacement(char *replace, struct msg_dir_entry *m)
{
	int done = FALSE;

	while(*replace) {
		char key = *replace++;
		char *p;
		char *cmd;
		NextChar(replace);

		switch(key) {
		case cTO:
			strcpy(m->to.name.str, macro_replace(get_string(&replace), m));
			m->to.name.sum = sum_string(m->to.name.str);
			if(debug_level & DBG_MSGTRANS)
				PRINTF("TRAN To: %s\n", m->to.name.str);
			break;

		case cFROM:
			strcpy(m->from.name.str, macro_replace(get_string(&replace), m));
			m->from.name.sum = sum_string(m->from.name.str);
			if(debug_level & DBG_MSGTRANS)
				PRINTF("TRAN From: %s\n", m->from.name.str);
			break;

		case cAT:
			strcpy(m->to.at.str, macro_replace(get_string(&replace), m));
			m->to.at.sum = sum_string(m->to.at.str);
			if(debug_level & DBG_MSGTRANS)
				PRINTF("TRAN At: %s\n", m->to.at.str);
			break;

		case cFLAG:
			switch(*replace++) {
			case 'P':
				SetMsgPersonal(m); break;
			case 'B':
				SetMsgBulletin(m); break;
			case 'T':
				SetMsgNTS(m); break;
			case 'S':
				SetMsgSecure(m); break;
			}
			NextSpace(replace);
			NextChar(replace);
			break;

		case cCOMMAND:
			cmd = get_string(&replace);

			if(!strcmp(cmd, "PRINT")) {
				if(!batch_mode && !ImBBS)
					PRINTF("%s", replace);
				while(*replace)
					replace++;
				break;
			}

			if(!strcmp(cmd, "HOLD")) {
				struct pending_operations *
					po = queue_pending_operation(HOLD);
				char *p = (char*) index(replace, ';');
				if(p != NULL) {
					p++;
					NextChar(p);
				} else
					p = "Held due to Translate";

				strcpy(po->name, "BBS");
				strcpy(po->text, p);
				po->number = ERROR;

				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Hold: Unconditionally\n");
				break;
			}

			if(!strcmp(cmd, "DONE")) {
				done = TRUE;
				break;
			}


			if(!strcmp(cmd, "NOFWD")) {
				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Nofwd:\n");
				SetMsgNoFwd(m);
				break;
			}

			if(!strcmp(cmd, "IMMUNE")) {
				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Immune:\n");
				SetMsgImmune(m);
				break;
			}

			if(!strcmp(cmd, "KILL")) {
				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Kill:\n");
				SetMsgKilled(m);
				break;
			}

			if(!strcmp(cmd, "OLD")) {
				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Old:\n");
				SetMsgOld(m);
				ClrMsgImmune(m);
				break;
			}

			if(!strcmp(cmd, "INIT")) {
				struct pending_operations *
					po = queue_pending_operation(INITIATE);
				strcpy(po->name, get_string(&replace));
				strcpy(po->text, m->sub);
				po->number = ERROR;

				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Init: %s (%s)\n", po->name, po->text);
				break;
			}	

			if(!strcmp(cmd, "COPY")) {
				struct pending_operations *
					po = queue_pending_operation(COPY);
				strcpy(po->name, get_string(&replace));
				po->number = ERROR;
				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Copy: %s\n", po->name);
				break;
			}

				/* What do we expect here? One of the following.
				 *   !MAIL bob@arasmith.com !KILL
				 *   !MAIL N0ARY !KILL
				 *   !MAIL !KILL
				 *         ^
				 *         +-- current location of "replace"
				 */

			if(!strcmp(cmd, "MAIL")) {
				struct pending_operations *
					po = queue_pending_operation(MAIL);

				po->name[0] = 0;

				if(*replace != '!' && *replace != 0) {
						/* at this point we have either an address or a call,
						 * look for next white to bracket the address.
						 */

					p = replace;
					NextSpace(replace);
					if(*replace != 0)
						*replace++ = 0;

					strcpy(po->name, p);
					if(debug_level & DBG_MSGTRANS)
						PRINTF("TRAN Mail: %s\n", po->name);
				}
				po->number = ERROR;

					/* we now have either an internet address or a call
					 * if we have a call that means we are supposed to
					 * look it up in the gateway.allow file. Or the field
					 * is blank indicating that the first line of the message
					 * contains our address.
					 */

				if(isCall(po->name)) {
					char buf[256];

					if(user_focus(po->name) == ERROR)
						break;

					strcpy(buf, user_get_field(uADDRESS));
					user_focus(usercall);

					if(!strncmp(buf, "NO,", 3))
						break;

					strcpy(po->name, buf);
					if(debug_level & DBG_MSGTRANS)
						PRINTF("TRAN Mail: %s\n", po->name);
				}
				break;
			}

			if(!strcmp(cmd, "WRITE")) {
				struct pending_operations *
					po = queue_pending_operation(WRITE);
				strcpy(po->name, macro_replace(get_string(&replace), m));
				po->number = ERROR;
				if(debug_level & DBG_MSGTRANS)
					PRINTF("TRAN Write: %s\n", po->name);
				break;
			}

		default:
			return FALSE;
		}
	}
	if(!(m->flags & MsgBid))
		if((m->flags & MsgBulletin) || !isCall(m->to.name.str) ||
			!isCall(m->from.name.str)) {
			strcpy(m->bid, "$");
			m->flags |= MsgBid;
		}
	return done;
}

