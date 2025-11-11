#include <stdio.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "tokens.h"
#include "function.h"
#include "user.h"
#include "message.h"
#include "pending.h"
#include "msg_list.h"
#include "bbscommon.h"

static int
	show_forwarding,
	show_deliverer,
	show_bids,
	show_groups,
	show_hloc,
	pattern_loc;

extern int debug_level;

int
	LastMsgListedCdate = 0;

static void
display_msg_flags(int mask)
{
	if(mask & MsgActive) PRINT("Active ");
	if(mask & MsgOld) PRINT("Old ");
	if(mask & MsgHeld) PRINT("Held ");
	if(mask & MsgKilled) PRINT("Killed ");
	if(mask & MsgImmune) PRINT("Immune ");
	if(mask & MsgPending) PRINT("Pending ");

	if(mask & MsgPersonal) PRINT("Pers ");
	if(mask & MsgBulletin) PRINT("Bull ");
	if(mask & MsgNTS) PRINT("NTS ");
	if(mask & MsgSecure) PRINT("Secure ");

	if(mask & MsgLocal) PRINT("Local ");
	if(mask & MsgCall) PRINT("Call ");
	if(mask & MsgCategory) PRINT("Category ");
	if(mask & MsgRead) PRINT("Read ");
	if(mask & MsgCheckedOut) PRINT("ChkOut ");
	if(mask & MsgSentFromHere) PRINT("FromHere ");
	if(mask & MsgNoFwd) PRINT("NoFwd ");

	if(mask & MsgMine) PRINT("Mine ");
	if(mask & MsgReadByMe) PRINT("ReadByMe ");
	if(mask & MsgClub) PRINT("Club ");
	if(mask & MsgSkip) PRINT("Skip ");
	if(mask & MsgAll) PRINT("All ");
	if(mask & MsgSentByMe) PRINT("SentByMe ");
	if(mask & MsgNew) PRINT("New ");
	if(mask & MsgNotRead) PRINT("NotRead ");
	if(mask & MsgNotReadByMe) PRINT("NotReadByMe ");
	if(mask & MsgReadByServer) PRINT("ReadByServer ");
}

static char *
range_type(int mask)
{
	switch(mask) {
	case LAST: return "LAST";
	case FIRST: return "FIRST";
	default: return "Unknown";
	}
}

static void
display_criteria(struct list_criteria *lc)
{
	PRINT("must_inc: ");
	display_msg_flags(lc->must_include_mask);
	PRINT("\n");

	PRINT("include: ");
	display_msg_flags(lc->include_mask);
	PRINT("\n");

	PRINT("exclude: ");
	display_msg_flags(lc->exclude_mask);
	PRINT("\n");

	PRINTF("range_type: %s [%d %d]\n", range_type(lc->range_type),
		lc->range[0], lc->range[1]);

	if(lc->pattern_type & MatchTOmask)
		PRINTF("pattern: TO [%s]\n", lc->pattern[MatchTO]);
	if(lc->pattern_type & MatchFROMmask)
		PRINTF("pattern: FROM [%s]\n", lc->pattern[MatchFROM]);
	if(lc->pattern_type & MatchATmask)
		PRINTF("pattern: AT [%s]\n", lc->pattern[MatchAT]);
	if(lc->pattern_type & MatchSUBmask)
		PRINTF("pattern: SUB [%s]\n", lc->pattern[MatchSUB]);

	if(lc->since)
		PRINTF("since: %d\n", lc->since);
}

static void
display_listing(struct list_criteria *lc, int to_pipe)
{
	struct msg_dir_entry *m;
	int cnt = lc->range[0];

	if(to_pipe == FALSE) {
		init_more();
		if(show_hloc)
			PRINT("Msg#  Stat  Size      To       From  Cnt Date/Time Hloc\n");
		else
			PRINT("Msg#  Stat  Size      To       From  Cnt Date/Time Subject\n");
		more();
	}	

	if(ImAscending) {
		if(lc->range_type == LAST) {
			cnt--;
			TAILQ_FOREACH_REVERSE(m, &MsgDirList, msg_dir_list, entries) {
				if (cnt == 0)
					break;
				if (TAILQ_PREV(m, msg_dir_list, entries) == NULL)
					break;
				if(m->visible)
					cnt--;
			}
		}
	} else {
		if(lc->range_type == FIRST) {
			TAILQ_FOREACH(m, &MsgDirList, entries) {
				if (cnt == 0)
					break;
				if (TAILQ_NEXT(m, entries) == NULL)
					break;
				if(m->visible)
					cnt--;
			}
		} else
			m = TAILQ_LAST(&MsgDirList, msg_dir_list);

		if(lc->must_include_mask & MsgNew)
			LastMsgListedCdate = m->cdate;
	}
	cnt = lc->range[0];

	while(m) {
		if(m->visible) {
			if(to_pipe)
				pipe_msg_num(m->number);
			else {
				char stat[5];
				char datebuf[20];
				struct tm *dt = localtime(&(m->cdate));

				if(IsMsgSecure(m)) stat[0] = 'S';
				else if(IsMsgPersonal(m)) stat[0] = 'P';
				else if(IsMsgBulletin(m)) stat[0] = 'B';
				else if(IsMsgNTS(m)) stat[0] = 'T';
				else stat[0] = '?';
	
				if(IsMsgPending(m)) stat[1] = 'P';
				else if(IsMsgNoFwd(m)) stat[1] = 'N';
				else stat[1] = ' ';

				if(IsMsgCheckedOut(m)) stat[2] = 'C';
				else if(IsMsgImmune(m)) stat[2] = 'I';
				else if(IsMsgOld(m)) stat[2] = 'O';
				else if(IsMsgHeld(m)) stat[2] = 'H';
				else if(IsMsgKilled(m)) stat[2] = 'K';
				else stat[2] = ' ';
	
				switch(m->flags & (MsgRead | MsgReadByMe)) {
				case MsgRead|MsgReadByMe:
					stat[3] = 'B'; break;
				case MsgRead:
					stat[3] = 'R'; break;
				case MsgReadByMe:
					stat[3] = 'M'; break;
				default:
					stat[3] = ' '; break;
				}
				stat[4] = 0;

				strftime(datebuf, 10, "%m%d/%H""%M", dt);
				if(show_bids) {
					PRINTF("%5d %5d %6s@%-6s %-6s %-.20s %-.28s\n",
						m->number, m->size, m->to.name.str,
						m->to.at.str, m->from.name.str, m->bid, m->sub);
				} else if(show_hloc) {
					PRINTF("%5d %s %5d %6s@%-6s %-6s%3d %9s %-.28s\n",
						m->number, stat, m->size, m->to.name.str,
						m->to.at.str, m->from.name.str, m->read_cnt,
						datebuf, m->to.address);
				} else
					PRINTF("%5d %s %5d %6s@%-6s %-6s%3d %9s %-.28s\n",
						m->number, stat, m->size, m->to.name.str,
						m->to.at.str, m->from.name.str, m->read_cnt,
						datebuf, m->sub);

				if(more()) {
					if(ImAscending) {
						m = TAILQ_LAST(&MsgDirList, msg_dir_list);
					} else {
						m = TAILQ_FIRST(&MsgDirList);
					}
				}
			}

			if(lc->range_type == LAST || lc->range_type == FIRST)
				cnt--;
		}

		if(ImAscending) {
			if(lc->must_include_mask & MsgNew)
				LastMsgListedCdate = m->cdate;
			if(lc->range_type == FIRST && cnt == 0)
				break;
			m = TAILQ_NEXT(m, entries);
		} else {
			if(lc->range_type == LAST && cnt == 0)
				break;
			m = TAILQ_PREV(m, msg_dir_list, entries);
		}
	}

	if(PendingOp != NULL)
		conditional_more();
}

int
msg_list_t(struct TOKEN *t)
{
	struct TOKEN *head = t;
	struct list_criteria lc;
	int cnt;
	int to_pipe = FALSE;
	int found_num = 0;

	show_forwarding = FALSE;
	show_deliverer = FALSE;
	show_bids = FALSE;
	show_hloc = FALSE;
	show_groups = FALSE;
	lc.pattern_type = FALSE;

	lc.must_include_mask = MsgSkip;
	lc.include_mask = MsgActive | MsgHeld;
	lc.exclude_mask = MsgKilled | MsgCheckedOut;

	lc.range_type = FALSE;
	lc.range[0] = 99999;
	lc.since = 0;

		/* special case the "L" command to mean LIST NEW */
	if(t->token == END)
		lc.must_include_mask |= MsgNew;

	while(t->token != END) {
		switch(t->token) {
		case PIPE:
			to_pipe = TRUE;
			break;
						/* pattern variables */
		case FROM:
			if(lc.pattern_type & MatchFROMmask)
				bad_cmd_double(110, pattern_loc, t->location);
			NEXT(t);
			pattern_loc = t->location;
			if(t->token != WORD && t->token != STRING)
				return bad_cmd(-1, t->location);

			lc.pattern_type |= MatchFROMmask;
			case_strcpy(lc.pattern[MatchFROM], t->lexem, AllUpperCase);
			lc.pattern_sum[MatchFROM] = sum_string(lc.pattern[MatchFROM]);
			break;

		case TO:
			if(lc.pattern_type & MatchTOmask)
				bad_cmd_double(110, pattern_loc, t->location);
			NEXT(t);
			pattern_loc = t->location;
			if(t->token != WORD && t->token != STRING)
				return bad_cmd(-1, t->location);

			lc.pattern_type |= MatchTOmask;
			case_strcpy(lc.pattern[MatchTO], t->lexem, AllUpperCase);
			lc.pattern_sum[MatchTO] = sum_string(lc.pattern[MatchTO]);
			break;

		case AT:
			if(lc.pattern_type & MatchATmask)
				bad_cmd_double(110, pattern_loc, t->location);
			NEXT(t);
			pattern_loc = t->location;
			if(t->token != WORD && t->token != STRING)
				return bad_cmd(-1, t->location);

			lc.pattern_type |= MatchATmask;
			case_strcpy(lc.pattern[MatchAT], t->lexem, AllUpperCase);
			lc.pattern_sum[MatchAT] = sum_string(lc.pattern[MatchAT]);
			break;

		case ABOUT:
			NEXT(t);
			if(t->token == END)
				return bad_cmd(-1, t->location);

		case STRING:
			if(lc.pattern_type & MatchSUBmask)
				bad_cmd_double(110, pattern_loc, t->location);
			pattern_loc = t->location;

			lc.pattern_type |= MatchSUBmask;
			case_strcpy(lc.pattern[MatchSUB], t->lexem, AllUpperCase);
			break;

		case NOPROMPT:
			no_prompt = TRUE;
			break;
		case WORD:
			if(lc.pattern_type & MatchSUBmask)
				bad_cmd_double(110, pattern_loc, t->location);
			pattern_loc = t->location;

			lc.pattern_type |= MatchSUBmask;
			case_strcpy(lc.pattern[MatchSUB], t->lexem, AllUpperCase);
			break;

						/* listing options */

		case BULLETIN:
			lc.must_include_mask |= MsgBulletin;
			break;
		case NTS:
			lc.must_include_mask |= MsgNTS;
			break;
		case SECURE:
			lc.must_include_mask |= MsgSecure;
			break;
		case PERSONAL:
			lc.must_include_mask |= MsgPersonal;
			break;

		case ALL:
			switch(msg_ListMode()) {
			case mMINE:
			case mSINCE:
				flush_message_list();
				msg_NormalMode();
			}

			lc.must_include_mask = 0;
			lc.include_mask = MsgActive | MsgHeld;
			if(ImSysop) {
				lc.include_mask |= MsgKilled;
				lc.exclude_mask &= ~MsgKilled;
			}
			break;

		case GROUP:
			show_groups = TRUE;
			break;

		case BID:
			show_bids = TRUE;
			break;

		case HLOC:
			show_hloc = TRUE;
			break;

		case FORWARD:
			show_forwarding = TRUE;
			break;

		case MINE:
			lc.must_include_mask |= MsgMine;
			break;
			
		case PENDING:
			lc.must_include_mask |= MsgPending;
			break;
		case HELD:
			lc.must_include_mask |= MsgHeld;
			lc.must_include_mask &= ~MsgSkip;
			break;
		case CHECK:
			show_deliverer = TRUE;
			lc.must_include_mask |= MsgCheckedOut;
			lc.exclude_mask &= ~MsgCheckedOut;
			break;
		case OLD:
			lc.must_include_mask |= MsgOld;
			break;
		case KILL:
			lc.must_include_mask |= MsgKilled;
			lc.include_mask = MsgKilled;
			lc.exclude_mask &= ~MsgKilled;
			break;
		case IMMUNE:
			lc.must_include_mask |= MsgImmune;
			lc.include_mask = MsgImmune;
			lc.exclude_mask &= ~MsgImmune;
			break;

		case READ:
			lc.must_include_mask &= ~MsgNotReadByMe;
			lc.must_include_mask |= MsgReadByMe;
			break;

		case UNREAD:
			lc.must_include_mask &= ~MsgReadByMe;
			lc.must_include_mask |= MsgNotReadByMe;
			break;

		case LOCAL:
			lc.must_include_mask |= MsgLocal;
			break;
		case INCLUDE:
			lc.must_include_mask |= MsgClub;
			break;
		case NEW:
			lc.must_include_mask |= MsgNew;
			break;

		case SINCE:
			{
				time_t tt = Time(NULL);
				struct tm *dt = localtime(&tt);
				int day;

				NEXT(t);
				switch(t->token) {
				case STRING:
					if(sscanf(
						t->lexem, "%d/%d", &(dt->tm_mon), &(dt->tm_mday)) != 2)
							return bad_cmd(-1, t->location);

					dt->tm_mon--;	
#ifdef SUNOS
					lc.since = timelocal(dt);
#else
					lc.since = mktime(dt);	/* was timelocal -- rwp */
#endif
					break;

				case WORD:
					case_string(t->lexem, AllUpperCase);
					if(!strncmp(t->lexem, "SUN", 3))
						day = 0;
					else if(!strncmp(t->lexem, "MON", 3))
						day = 1;
					else if(!strncmp(t->lexem, "TUE", 3))
						day = 2;
					else if(!strncmp(t->lexem, "WED", 3))
						day = 3;
					else if(!strncmp(t->lexem, "THU", 3))
						day = 4;
					else if(!strncmp(t->lexem, "FRI", 3))
						day = 5;
					else if(!strncmp(t->lexem, "SAT", 3))
						day = 6;

					if(day <= dt->tm_wday)
						day = dt->tm_wday - day;
					else
						day = (dt->tm_wday + 7) - day;

					dt->tm_sec = dt->tm_min = dt->tm_hour = 0; 
#ifdef SUNOS
					lc.since = timelocal(dt) - (day * tDay);
#else
					lc.since = mktime(dt) - (day * tDay); /* was timelocal -- rwp */
#endif
					break;

				default:
					return bad_cmd(-1, t->location);
				}
			}
			break;
				
		case FIRST:
			if(lc.range_type)
				return bad_cmd(111, t->location);
			lc.range_type = FIRST;
			lc.range[0] = 10;
			break;
		case LAST:
			if(lc.range_type)
				return bad_cmd(111, t->location);
			lc.range_type = LAST;
			lc.range[0] = 10;
			break;

		case UNKNOWN:
				return bad_cmd(112, t->location);
		}

		NEXT(t);
	}

	t = head;

	while(t->token != END) {
		switch(t->token) {
						/* range or count variable */
		case NUMBER:
			switch(lc.range_type) {
			case FALSE:
			case FULL:
				switch(found_num) {
				case 0:
					lc.range[0] = t->value;
					lc.range[1] = 99999;
					break;
				case 1:
					lc.range[1] = t->value;
					break;
				default:
					return bad_cmd(114, t->location);
				}
				found_num++;
				lc.range_type = FULL;
				break;
			case FIRST:
			case LAST:
				if(found_num++)
					return bad_cmd(113, t->location);
				lc.range[0] = t->value;	
			}
			break;

		case DASH:
			switch(lc.range_type) {
			case FALSE:
				switch(found_num) {
				case 0:
					found_num++;
					lc.range[0] = 0;
					break;
				case 1:
					break;
				case 2:
					return bad_cmd(115, t->location);
				}
				break;

			case FIRST:
			case LAST:
				return bad_cmd(113, t->location);
			}
		}
		NEXT(t);
	}

	if(debug_level & DBG_MSGLIST)
		display_criteria(&lc);
	
	if((cnt = build_partial_list(&lc)) == 0)
		return system_msg(116);

	if(lc.range_type == FIRST || lc.range_type == LAST)
		if(lc.range[0] > cnt)
			lc.range_type = FALSE;

	if(to_pipe || check_long_xfer_abort(117, cnt) == OK)
		display_listing(&lc, to_pipe);
	else 
		if(lc.must_include_mask & MsgNew) {
			struct msg_dir_entry *m;
			TAILQ_FOREACH(m, &MsgDirList, entries) {
				if(m->visible)
					LastMsgListedCdate = m->cdate;
			}
		}
		
	return OK;
}

int
msg_catchup(void)
{
	if(LastMsgListedCdate == 0) {
		struct list_criteria lc;
		struct msg_dir_entry *m;

		bzero(&lc, sizeof(lc));
		lc.pattern_type = FALSE;
		lc.must_include_mask = MsgSkip;
		lc.include_mask = MsgActive;
		lc.exclude_mask = MsgKilled | MsgCheckedOut;
		lc.range_type = FALSE;
		lc.must_include_mask |= MsgNew;

		if(build_partial_list(&lc) == 0)
			return system_msg(116);

		TAILQ_FOREACH(m, &MsgDirList, entries) {
			if (TAILQ_NEXT(m, entries) == NULL)
				break;
			if(m->visible)
				LastMsgListedCdate = m->cdate;
		}
	}

	user_set_value(uMESSAGE, LastMsgListedCdate);
	return OK;
}
