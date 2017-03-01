#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "user.h"
#include "message.h"
#include "vars.h"
#include "version.h"

void
	remove_token(struct TOKEN *t);

extern void
    msg_build_mid(void);
    

struct text_line *GrpList = NULL;

int active_message;
int last_message_listed;

char Group[20];

#define IsFOUND(x,y,z)	{ if(msg->flags&x)\
				if(!ImBBS) {bad_cmd(151,t->location); return z;}\
				msg->flags|=y; }

static void
display_groups(void)
{
	struct text_line *tl;
	struct text_line *headers = NULL;
	struct text_line *output = NULL;

	textline_sort(&GrpList, 5);

	textline_append(&headers, " Cnt Group");
	textline_append(&headers, " --- -----");
	disp_column_wise(&output, headers, GrpList);

	tl = output;
	while(tl) {
		PRINTF("%s\n", tl->s);
		NEXT(tl);
	}
	textline_free(headers);
	textline_free(output);
}

static void
create_group_list(char *s)
{
	char *name = get_string(&s);
	int cnt = get_number(&s);
	char buf[80];

	if(cnt) {
		sprintf(buf, "%4d %s", cnt, name);
		textline_append(&GrpList, buf);
	}
}

int
msg(void)
{
	struct TOKEN *t = TokenList;

	int found_num = FALSE;
	int token = t->token;
	char *p;

	NEXT(t);

	switch(token) {
	case GROUP:
		switch(t->token) {
		case OFF:
			Group[0] = 0;
			msgd_cmd_string(msgd_xlate(mGROUP), "OFF");
			flush_message_list();
			break;
		case WORD:
		case STRING:
			p = t->lexem;
			strncpy(Group, get_string(&p), 19);
			case_string(Group, AllUpperCase);
			msgd_cmd_string(msgd_xlate(mGROUP), Group);
			flush_message_list();
			PRINTF("GROUP: %s\n", Group);
			break;
		case END:
			textline_free(GrpList);
			GrpList = NULL;
			msgd_fetch_multi(msgd_xlate(mGROUP), create_group_list);
			display_groups();
			PRINTF("\nCurrent GROUP: %s\n", Group[0] ? Group : "None");
			break;
		}
		return OK;

	case READ:
		msg_read_t(t);
		return OK;

	case ROUTE:
		msg_chk_route_t(t);
		break;

	case REVFWD:
		if(ImBBS)
			msg_revfwd();
		return OK;

	case KILL:
		msg_kill_t(t);
		return OK;

	case IMMUNE:
		msg_immune_t(IMMUNE, t);
		return OK;

	case REFRESH:
		msg_immune_t(REFRESH, t);
		return OK;

	case SEND:
		msg_snd_t(t);
		return OK;

	case LIST:
		switch(t->token) {
		case ASCEND:
			if(ImAscending)
				PRINT("Messages already listed in ASCENDING order\n");
			else {
				flush_message_list();
				PRINT("Message listing will be in ASCENDING order\n");
				user_set_flag(uASCEND);
			}
			break;
		case DESCEND:
			if(ImDescending)
				PRINT("Messages already listed in DESCENDING order\n");
			else {
				flush_message_list();
				PRINT("Message listing will be in DESCENDING order\n");
				user_clr_flag(uASCEND);
			}
			break;
		case CATCHUP:
			msg_catchup();
			break;

		default:
			msg_list_t(t);
		}
		return OK;

	case CHECK:
		msg_check_t(t);
		return OK;
		
	case COPY:
		msg_copy_t(t);
		return OK;
		
	case EDIT:
		if(ImBBS)
			return ERROR;
		msg_edit();
		return OK;

	case REPLY:
		msg_rply_t(t);
		return OK;

	case HOLD:
		{
			int op = t->last->token;
			struct TOKEN *top = t;
			found_num = 0;

			while(t->token != END) {
				if(t->token != NUMBER)
					return bad_cmd(79, t->location);
				found_num++;
				NEXT(t);	
			}
	
			/* if there is no number operand than assign active */
			if(!found_num)
				if(append_active_message())
					return error(77);
			return msg_alter_state(op, top);
		}

	case RELEASE:
	case ACTIVATE:
		{
			int op = t->last->token;
			struct TOKEN *top = t;
			int held = FALSE;
			found_num = 0;

			if(ImBBS)
				return ERROR;

			while(t->token != END) {
				switch(t->token) {
					case NUMBER:
						found_num++;
						break;
					case HOLD:
						held = TRUE;
						break;
				}
				NEXT(t);	
			}
	
			/* if there is no number operand than assign active */
			if(!found_num && !held)
				if(append_active_message())
					return error(77);
			return msg_alter_state(op, top);
		}

	default:
		PRINTF("Bad token in msg()\n");
		return ERROR;
	}

	return OK;
}

int
append_active_message(void)
{
	struct TOKEN *tmp, *t = TokenList;

	while(t->token != END)
		NEXT(t);

	tmp = t;
	PREV(t);

	t = grab_token_struct(t);
	t->next = tmp;
	tmp->last = t;

	t->token = NUMBER;
	t->value = active_message;
	return OK;	
}

struct TOKEN *
match_address(struct TOKEN *t, struct msg_dir_entry *msg)
{
	while(t->token != END) {
		switch(t->token) {
		case ADDRESS:		/* . */
			NEXT(t);
			switch(t->token) {
			case LOCALE:	/* # */
				NEXT(t);
				case_string(t->lexem, AllUpperCase);
				if(msg->to.address[0])
					sprintf(msg->to.address, "%s.#%s", msg->to.address, t->lexem);
				else
					sprintf(msg->to.address, "#%s", t->lexem);
				msg->flags |= MsgHloc;
				break;

			case WORD:
			case NUMBER:
				case_string(t->lexem, AllUpperCase);
				if(msg->to.address[0])
					sprintf(msg->to.address, "%s.%s", msg->to.address, t->lexem);
				else
					sprintf(msg->to.address, "%s", t->lexem);
				msg->flags |= MsgHloc;
				break;

			default:
					return t;
			} 
			break;

		case LOCALE:		/* # */
			NEXT(t);
			case_string(t->lexem, AllUpperCase);
			if(msg->to.address[0])
				sprintf(msg->to.address, "%s.#%s", msg->to.address, t->lexem);
			else
				sprintf(msg->to.address, "#%s", t->lexem);
			msg->flags |= MsgHloc;
			break;

		case WORD:
		case NUMBER:
			case_strcpy(msg->to.at.str, t->lexem, AllUpperCase);
			if(isCall(msg->to.at.str))
				IsFOUND(MsgAtMask, MsgAtBbs, NULL)
			else
				IsFOUND(MsgAtMask, MsgAtDist, NULL)
			break;

		case SSID:			/* - */
						/* strip possible ssid on to call */
			if(t->next)
				if(t->next->token == NUMBER && t->last->token == AT) {
					NEXT(t);
					NEXT(t);
					continue;
				}
			t->token = DASH;
			return t;

		default:
			return t;
		}
		NEXT(t);
	}
	return t;
}

#if 0
When we are connected to a bbs we need to disable our fancy parser on the
send command. The way it is we misinterprete stuff that may come along
in other fields.
#endif

int
msg_bbs_xlate_tokens(struct msg_dir_entry *msg)
{
	char *s = CmdLine;
	char cmdline[256];

	sprintf(cmdline, "%s: %s", usercall, CmdLine);
	uppercase(s);
	
	/* SP N6ZFJ @ N0ARY.#NOCAL.CA.USA.NOAM < N6QMY $00001_N6QMY */

	if(*s++ != 'S') {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "msg_bbs_xlate_tokens", cmdline,
				   "Expected command to start with an 'S'");
		exit(1);
	}

	msg->flags &= ~MsgTypeMask;
	switch(*s++) {
	case 'B':
		msg->flags |= MsgBulletin;
		break;
	case 'T':
		msg->flags |= MsgNTS;
		break;
	case 'P':
		msg->flags |= MsgPersonal;
		break;
	default:
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "msg_bbs_xlate_tokens", cmdline,
				   "Expected 'B', 'T' or 'P' message type");
		exit(1);
	}

	NextChar(s);

	/* SP N6ZFJ @ N0ARY.#NOCAL.CA.USA.NOAM < N6QMY $00001_N6QMY
	 *    ^
	 * Now we are setting at the TO field 
	 */

	if(*s == '@') {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "msg_bbs_xlate_tokens", cmdline,
				   "Expected a TO field");
		exit(1);
	}

	strcpy(msg->to.name.str, get_string(&s));

	if(msg->flags & MsgNTS)
		msg->flags |= MsgToNts;
	else {
		if(isCall(msg->to.name.str))
			msg->flags |= MsgCall;
		else
			msg->flags |= MsgCategory;
	}

	if(*s == '@') {
		s++;
		NextChar(s);

	/* SP N6ZFJ @ N0ARY.#NOCAL.CA.USA.NOAM < N6QMY $00001_N6QMY
	 *            ^
	 * Now we are at the AT field 
	 */

		strcpy(msg->to.at.str, get_call(&s));
		if(isCall(msg->to.at.str))
			msg->flags |= MsgAtBbs;
		else
			msg->flags |= MsgAtDist;

		switch(*s) {
		case '.':
			s++;
			strcpy(msg->to.address, get_string(&s));
			msg->flags |= MsgHloc;
			break;
		case '<':
			break;
		default:
			/* possible SSID here? */
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   	"msg_bbs_xlate_tokens", cmdline,
				   	"Expected a space or period after AT field");
			exit(1);
		}
	}

	/* SP N6ZFJ @ N0ARY.#NOCAL.CA.USA.NOAM < N6QMY $00001_N6QMY
	 *                                     ^
	 * Now we are at the FROM field 
	 */

	if(*s != '<') {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "msg_bbs_xlate_tokens", cmdline,
				   "Expected the '<' token to follow AT field");
		exit(1);
	}

	s++;
	NextChar(s);
	strcpy(msg->from.name.str, get_string(&s));
	msg->flags |= MsgFrom;

	/* SP N6ZFJ @ N0ARY.#NOCAL.CA.USA.NOAM < N6QMY $00001_N6QMY
	 *                                             ^
	 * Now we are at the BID field, if it is present.
	 */

	if(*s == '$') {
		s++;
		NextChar(s);

		if(*s == 0) {
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
					   "msg_bbs_xlate_tokens", cmdline,
					   "Expected the BID to follow '$' token");
			exit(1);
		}
		strcpy(msg->bid, get_string(&s));
		msg->flags |= MsgBid;
	}

	if(*s != 0) {
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				   "msg_bbs_xlate_tokens", cmdline,
				   "Expected NULL terminator");
		exit(1);
	}
	return OK;
}
	
int
msg_xlate_tokens(struct TOKEN *t, struct msg_dir_entry *msg)
{
	char *tp;
	int done = FALSE;
	int asked = FALSE;
	bzero(msg, sizeof(struct msg_dir_entry));

	if(ImBBS)
		return msg_bbs_xlate_tokens(msg);

	while(!done) {
		while(t->token != END) {
			switch(t->token) {
			case DISTRIB:
				break;

			case BULLETIN:
				IsFOUND(MsgTypeMask, MsgBulletin, ERROR)
				break;
			case PERSONAL:
				IsFOUND(MsgTypeMask, MsgPersonal, ERROR)
				break;
			case SECURE:
				IsFOUND(MsgTypeMask, MsgSecure, ERROR)
				break;
			case NTS:
				if((msg->flags & MsgTypeMask) == 0) {
					IsFOUND(MsgTypeMask, MsgNTS, ERROR)
					break;
				}
						/* possiblities for the TO field, CALL, CATEGORY, ZIP, DISTRIBUTION */
			case WORD:
			case STRING:
					/* If SECURE and we already have a TO supplied than assume this
					 * word is a PASSWORD. If we already have a PASSWORD then complain
					 * and ignore the word.
					 */
				if(msg->flags & MsgSecure && msg->flags & MsgToMask) {
					if(msg->flags & MsgPassword) {
						bad_cmd(156, t->location);
						break;
					}
						
					t->lexem[8] = 0;
					case_strcpy(msg->passwd, t->lexem, AllUpperCase);
					msg->flags |= MsgPassword;
					break;
				}

				t->lexem[6] = 0;

				if(msg->flags & MsgToMask) {
					bad_cmd(151, t->location);
					break;
				}

				case_strcpy(msg->to.name.str, t->lexem, AllUpperCase);
				if(isCall(msg->to.name.str))
					msg->flags |= MsgCall;
				else {
					if(!strncmp(msg->to.name.str, "NTS", 3))
						msg->flags |= MsgToNts;
					else
						msg->flags |= MsgCategory;
				}
				t->token = TO;	/* necessary for detecting SSID */
				break;
	
			case FROM:
				IsFOUND(MsgFrom, MsgFrom, ERROR)
				NEXT(t);
				if(t->token != WORD)
					return bad_cmd(151, t->location);
				t->lexem[6] = 0;
				case_strcpy(msg->from.name.str, t->lexem, AllUpperCase);
				break;
	
	
			case BID:
				IsFOUND(MsgBid, MsgBid, ERROR)
				NEXT(t);
				tp = &CmdLine[t->location];
				case_strcpy(msg->bid, get_string(&tp), AllUpperCase);
				while(t->token != END) {
					if(tp == &CmdLine[t->location])
						break;
					NEXT(t);
				}
				continue;
	
			case NUMBER:
				IsFOUND(MsgToMask, MsgToNts, ERROR)
				t->lexem[6] = 0;
				strcpy(msg->to.name.str, t->lexem);
				break;
	
			case AT:
				NEXT(t);
				if((t = match_address(t, msg)) == NULL)
					return ERROR;
				continue;
	
			case SSID:
						/* strip possible ssid on to call */
				if(t->next)
					if(t->next->token == NUMBER && t->last->token == TO) {
						NEXT(t);
						NEXT(t);
						continue;
					}
				return bad_cmd(151, t->location);
	
			default:
					/* If there is a bbs connected then don't error out on
					 * stupid tokens. We have no idea what letters are going
					 * to be allowed in the TO fields of other bbs's.
					 */
				if(!ImBBS)
					return bad_cmd(151, t->location);

					/* See if we can identify the next valid token at this
					 * time. This is our only reliable way to again sync up.
					 */

				while(TRUE) {
					int token = t->next->token;
					int found = FALSE;

					switch(token) {
					case FROM:
					case AT:
					case BID:
					case PASSWORD:
					case END:
						found = TRUE;
					}

					if(found)
						break;

					NEXT(t);
				}
			}
	
			NEXT(t);
		}
		done = TRUE;
	
		if((msg->flags & (MsgToMask | MsgNTS)) == 0) {
			char buf[80];
			if(asked)
				return error(195);
			asked = TRUE;
			PRINT("Send To: ");
			if(NeedsNewline)
				PRINT("\n");
			GETS(buf, 79);
			if(buf[0] == 0)
				return error(195);
			parse_simple_string(buf, SendOperands);
			t = TokenList;
			done = FALSE;
		}
	}
	return OK;
}

static void
msg_activate_all_held(void)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;
	int cnt;

	lc.pattern_type = FALSE;
	lc.must_include_mask = MsgHeld;
	lc.include_mask = MsgActive;
	lc.exclude_mask = MsgKilled | MsgCheckedOut;
	lc.range_type = FALSE;

	if((cnt = build_partial_list(&lc)) == 0) {
		PRINT("No held messages\n");
		return;
	}

	PRINTF("cnt=%d\n", cnt);
	m = MsgDirList;
	while(m) {
		if(m->visable)
			msgd_cmd_num(msgd_xlate(mACTIVE), m->number);
		NEXT(m);
	}
}

int
msg_alter_state(int op, struct TOKEN *t)
{
	struct msg_dir_entry *m;

	while(t->token != END) {
		switch(t->token) {
		case NUMBER:
			switch(op) {
			case RELEASE:
			case ACTIVATE:
				msg_hold_release(t->value);
				break;
			case HOLD:
				if((m = msg_locate(t->value)) == NULL)
					return error_number(132, t->value);
				msg_hold_get_reason(m, NULL);
				break;
			}
			break;
		case HOLD:
			msg_activate_all_held();
			break;
		}
		NEXT(t);
	}
	return OK;
}
