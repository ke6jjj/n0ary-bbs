#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "user.h"
#include "message.h"
#include "vars.h"

#define CHECK_STATE {\
	if(operand_pending) {\
		bad_cmd(-1, t->location);\
		PRINTF("*** Expected operand, operands need to be in quotes\n");\
		return ERROR;\
	}\
	if(rfc) {\
		bad_cmd(-1, t->location);\
		PRINTF("*** Second function encountered\n");\
		return ERROR;\
	}}

#define LOOK_FORWARD \
	if(t->next->token != STRING) {\
		bad_cmd(-1, t->next->location);\
		PRINTF("*** Expected an operand, operands need to be in quotes\n");\
		return ERROR;\
	}

static int noprompt;

struct msg_list {
	struct msg_list *next;
	int num;
} *Msg_List = NULL;

msg_edit(void)
{
	struct TOKEN *t = TokenList;
	struct msg_dir_entry msg;
	struct msg_list *ml;
	int msg_cnt = 0;
	int function = 0;
	int rfc = 0;
	int operand_pending = FALSE;
	int need_bid = FALSE;
	int kill_bid = FALSE;
	int raise_case = FALSE;
	char value[256], cmd[256];

	noprompt = FALSE;
	NEXT(t);

	while(t) {
		switch(t->token) {
		case NOPROMPT:
			noprompt = TRUE;
			break;

		case NUMBER:
			ml = malloc_struct(msg_list);
			ml->next = Msg_List;
			ml->num = t->value;
			Msg_List = ml;
			msg_cnt++;
			break;

		default:
			bad_cmd(-1, t->location);
			if(rfc == 0)
				PRINT("*** Operand present without function\n");
			if(!operand_pending)
				PRINT("*** Operand not expected\n");
			return ERROR;

		case STRING:
			if(rfc == 0) {
				bad_cmd(-1, t->location);
				PRINT("*** Operand present without function\n");
				return ERROR;
			}

			if(!operand_pending) {
				bad_cmd(-1, t->location);
				PRINT("*** Operand not expected\n");
				return ERROR;
			}
			strcpy(value, t->lexem);
			if(raise_case)
				uppercase(value);
			operand_pending = FALSE;
			break;

		case BID:
			CHECK_STATE
			LOOK_FORWARD
			operand_pending = TRUE;	
			rfc = rBID;
			kill_bid = TRUE;
			break;

		case SUBJECT:
			CHECK_STATE
			LOOK_FORWARD
			operand_pending = TRUE;	
			rfc = rSUBJECT;
			break;

		case PASSWORD:
			CHECK_STATE
			LOOK_FORWARD
			operand_pending = TRUE;	
			rfc = rPASSWORD;
			raise_case = TRUE;
			break;

		case TO:
			CHECK_STATE
			LOOK_FORWARD
			operand_pending = TRUE;	
			rfc = rTO;
			function = TO;
			break;

		case AT:
			CHECK_STATE
			LOOK_FORWARD
			operand_pending = TRUE;	
			rfc = rTO;
			function = AT;
			break;

		case FROM:
			CHECK_STATE
			LOOK_FORWARD
			operand_pending = TRUE;	
			rfc = rFROM;
			break;

		case PERSONAL:
			CHECK_STATE
			rfc = rTYPE;
			strcpy(value, "P");
			kill_bid = TRUE;
			break;

		case BULLETIN:
			CHECK_STATE
			rfc = rTYPE;
			strcpy(value, "B");
			need_bid = TRUE;
			break;

		case NTS:
			CHECK_STATE
			rfc = rTYPE;
			strcpy(value, "T");
			kill_bid = TRUE;
			break;

		case SECURE:
			CHECK_STATE
			rfc = rTYPE;
			strcpy(value, "S");
			break;

		case END:
			break;

		}
		NEXT(t);
	}

	if(msg_cnt == 0) {
		if(active_message == 0) {
			PRINTF("*** You must supply a message number, no active message.\n");
			return ERROR;
		}
		ml = malloc_struct(msg_list);
		ml->next = Msg_List;
		ml->num = active_message;
		Msg_List = ml;
		msg_cnt++;
	}

	switch(function) {
	case TO:
		parse_simple_string(value, CallOperands);
		msg_xlate_tokens(TokenList, &msg);

		switch(msg.flags & MsgToMask) {
		case MsgCategory:
			if(msg.flags & MsgAtDist) {
				need_bid = TRUE;
				break;
			}
		case MsgCall:
			if(determine_homebbs(&msg))
				return OK;
			break;
		}
		if(msg.flags & MsgHloc)
			sprintf(cmd, "%s %s@%s.%s", rfc822_xlate(rTO),
				msg.to.name.str, msg.to.at.str, msg.to.address);
		else
			sprintf(cmd, "%s %s@%s", rfc822_xlate(rTO),
				msg.to.name.str, msg.to.at.str);
		break;
	case AT:
		break;
	default:
		sprintf(cmd, "%s %s", rfc822_xlate(rfc), value);
	}

	while(Msg_List) {
		struct msg_dir_entry *m;
		ml = Msg_List;
		NEXT(Msg_List);

		if((m = msg_locate(ml->num)) != NULL) {
			if(rfc == 0) {
				msg_edit_prompt(m);
				msg_hold_release(m->number);
			} else {
				if(kill_bid && (m->flags & MsgBid)) {
					char lcmd[256];
					bid_delete(m->bid);
					sprintf(lcmd, "%s", rfc822_xlate(rBID));
					PRINTF("%d \"%s\" %s\n", ml->num, lcmd,
						(msg_edit_issue(ml->num, lcmd) == OK)?"OK":"ERROR");
					m->flags &= ~MsgBid;
				}
				if(function == AT) {
					char buf[80];
					sprintf(buf, "%s@%s", m->to.name.str, value);
					parse_simple_string(buf, CallOperands);
					msg_xlate_tokens(TokenList, &msg);
			
					switch(msg.flags & MsgToMask) {
					case MsgCategory:
						if(msg.flags & MsgAtDist) {
							need_bid = TRUE;
							break;
						}
					case MsgCall:
						if(determine_homebbs(&msg))
							return OK;
						break;
					}
					if(msg.flags & MsgHloc)
						sprintf(cmd, "%s %s@%s.%s", rfc822_xlate(rTO),
							msg.to.name.str, msg.to.at.str, msg.to.address);
					else
						sprintf(cmd, "%s %s@%s", rfc822_xlate(rTO),
							msg.to.name.str, msg.to.at.str);
				}
				if(msg_edit_issue(ml->num, cmd)) {
					PRINTF("%d \"%s\" ERROR\n", ml->num, cmd);
					if(rfc == rBID)
						need_bid = TRUE;
				} else {
					PRINTF("%d \"%s\" OK\n", ml->num, cmd);
					if(rfc == rBID)
						m->flags |= MsgBid;
				}

				if(need_bid && !(m->flags & MsgBid)) {
					char lcmd[256];
					sprintf(lcmd, "%s %s", rfc822_xlate(rBID), "$");
					PRINTF("%d \"%s\" %s\n", ml->num, lcmd,
						(msg_edit_issue(ml->num, lcmd) == OK)?"OK":"ERROR");
				}
			}
		} else
			system_msg_number(132, ml->num);
	}
	return OK;
}

int
msg_edit_prompt(struct msg_dir_entry *m)
{
	struct msg_dir_entry msg;
	char c, buf[80];
	int changed = FALSE;
	int bid_changed = FALSE;
	char bid[80];
	char cmd[256];
	int orig_flags = m->flags;

	strcpy(bid, m->bid);

			/* at this point we could have any of the following:
			 *   N6ZFJ			(generate address from wp)
			 *   N6ZFJ@N0ARY	(generate bbs address from wp)
			 *   N6ZFJ@N0ARY.#NOCAL.CA.USA.NA	(override wp)
			 *
			 * reparse the last part of the command looking for
			 * address info.
			 */

	buf[0] = 0;
	if(!noprompt) {
		PRINTF("To [%s @ %s]? ", m->to.name.str, m->to.at.str);
		if(NeedsNewline)
			PRINT("\n");
		GETS(buf, 79);
	}

	if(buf[0]) {
		changed = TRUE;
		parse_simple_string(buf, CallOperands);
		msg_xlate_tokens(TokenList, &msg);

		m->flags &= ~(MsgToMask|MsgAtMask|MsgHloc);
		m->flags |= (msg.flags & (MsgToMask|MsgAtMask|MsgHloc));

		strcpy(m->to.name.str, msg.to.name.str);
		strcpy(m->to.at.str, msg.to.at.str);

		if(m->flags & MsgHloc)
			strcpy(m->to.address, msg.to.address);

		switch(m->flags & MsgToMask) {
		case MsgToNts:
			break;
		case MsgCategory:
			if(!(m->flags & MsgAtBbs)) {
				m->to.address[0] = 0;
				m->flags &= ~MsgHloc;
				changed = TRUE;

				if(!(m->flags & MsgBid)) {
					bid_changed = TRUE;
					strcpy(bid, "$");
				}
				break;
			}
		case MsgCall:
			if(determine_homebbs(m))
				return OK;
			if(!(m->flags & MsgAtBbs))
				if(!(m->flags & MsgBid)) {
					bid_changed = TRUE;
					strcpy(bid, "$");
				}
			break;
		}
	}

	buf[0] = 0;
	if(!noprompt) {
		PRINTF("Hloc [%s]? ", m->to.address);
		if(NeedsNewline)
			PRINT("\n");
		GETS(buf, 79);
	}

	if(buf[0]) {
		changed = TRUE;
		if(buf[0] == ' ') {
			m->to.address[0] = 0;
			m->flags &= ~MsgHloc;
		} else {
			case_string(buf, AllUpperCase);
			strncpy(m->to.address, buf, LenHLOC);
			m->flags |= MsgHloc;
		}
	}

	if(changed) {
		if(m->flags & MsgHloc)
			sprintf(cmd, "%s %s@%s.%s", rfc822_xlate(rTO),
				m->to.name.str, m->to.at.str, m->to.address);
		else
			sprintf(cmd, "%s %s@%s", rfc822_xlate(rTO),
				m->to.name.str, m->to.at.str);
		msg_edit_issue(m->number, cmd);
	}

	m->flags &= ~MsgTypeMask;
	m->flags |= (orig_flags & MsgTypeMask);

	switch(m->flags & MsgTypeMask) {
	case MsgPersonal:
		c = 'P'; break;
	case MsgBulletin:
		c = 'B'; break;
	case MsgNTS:
		c = 'T'; break;
	case MsgSecure:
		c = 'S'; break;
	}

	buf[0] = 0;
	if(!noprompt) {
		PRINTF("Type [%c]? ", c);
		if(NeedsNewline)
			PRINT("\n");
		GETS(buf, 79);
	}

	if(buf[0]) {
		changed = TRUE;
		switch(buf[0]) {
		case 'P':
		case 'p':
			SetMsgPersonal(m);
			sprintf(cmd, "%s P", rfc822_xlate(rTYPE));
			msg_edit_issue(m->number, cmd);
			break;
		case 'b':
		case 'B':
			SetMsgBulletin(m);
			if(!(m->flags & MsgBid)) {
				bid_changed = TRUE;
				strcpy(bid, "$");
			}
			sprintf(cmd, "%s B", rfc822_xlate(rTYPE));
			msg_edit_issue(m->number, cmd);
			break;
		case 'T':
		case 't':
			SetMsgNTS(m);
			sprintf(cmd, "%s T", rfc822_xlate(rTYPE));
			msg_edit_issue(m->number, cmd);
			break;
		case 'S':
		case 's':
			SetMsgSecure(m);
			PRINTF("Password? ");
			if(NeedsNewline)
				PRINT("\n");
			GETS(buf, 79);
			if(buf[0])
				SetMsgPassword(m, buf);
			sprintf(cmd, "%s S", rfc822_xlate(rTYPE));
			msg_edit_issue(m->number, cmd);
			sprintf(cmd, "%s %s", rfc822_xlate(rPASSWORD), buf);
			msg_edit_issue(m->number, cmd);
			break;
		}
	}

	buf[0] = 0;
	if(!noprompt) {
		PRINTF("Sub [%s]? ", m->sub);
		if(NeedsNewline)
			PRINT("\n");
		GETS(buf, 79);
	}

	if(buf[0]) {
		changed = TRUE;
		strncpy(m->sub, buf, 60);
		sprintf(cmd, "%s %s", rfc822_xlate(rSUBJECT), buf);
		msg_edit_issue(m->number, cmd);
	}

	buf[0] = 0;
	if(!noprompt) {
		PRINTF("Bid [%s]? ", (bid[0] == '$')?"generated":bid);
		if(NeedsNewline)
			PRINT("\n");
		GETS(buf, 79);
	}

	if(buf[0]) {
		strncpy(bid, buf, LenBID);
		case_string(bid, AllUpperCase);
		bid_changed = TRUE;
	}

	if(bid_changed) {
		sprintf(cmd, "%s %s", rfc822_xlate(rBID), bid);
		if(msg_edit_issue(m->number, cmd) == OK) {
			m->flags |= MsgBid;
			strcpy(m->bid, bid);
		} else
			PRINTF("The bid \"%s\" is a duplicate, bid unchanged\n", bid);

		if(bid[0] == ' ')
			m->flags &= ~MsgBid;
		changed = TRUE;
	}

	if(!changed) {
		if(noprompt)
			changed = TRUE;
		else {
			PRINTF("Activate [Y/n]: ");
			if(get_yes_no(YES) == YES)
				changed = TRUE;
		}
	}

	if(changed && strcmp(m->to.at.str, Bbs_Call))
		field_translation(m);
	return changed;
}
