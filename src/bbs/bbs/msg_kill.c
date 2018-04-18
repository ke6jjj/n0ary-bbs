#include <stdio.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "user.h"
#include "message.h"
#include "vars.h"
#include "msg_kill.h"
#include "help.h"

static void kill_mine(void);
static int kill_range(int start, int finish);
static void kill_messages(struct TOKEN *t);
static int kill_message_number(struct msg_dir_entry *m);

static int
	inquire,
	hard_delete,
	mine;

int
msg_kill_t(struct TOKEN *t)
{
	int msg_cnt = 0;
	int range[2], dorange = FALSE;

	range[1] = 0;
	mine = FALSE;
	hard_delete = FALSE;
	inquire = (batch_mode) ? FALSE:TRUE;

	while(t->token != END) {
		switch(t->token) {
		case MINE:
			if(msg_cnt)
				return bad_cmd(141, t->location);
			mine = TRUE;
			break;
		case HARD:
			if(ImSysop)
				hard_delete = TRUE;
			break;
		case NUMBER:
			if(range[1] | mine)
				return bad_cmd(141, t->location);
			msg_cnt++;
			range[dorange] = t->value;
			break;

		case NOPROMPT:
			if(!ImSysop)
				return bad_cmd(78, t->location);
			inquire = FALSE;
			break;

		case DASH:
			if(!ImSysop)
				return bad_cmd(78, t->location);

			if(range[0] == 0)
				return bad_cmd(140, t->location);
			dorange++;
			break;

		case COMMA:
			break;

		default:
			return bad_cmd(140, t->location);
		}
		NEXT(t);
	}

	if(mine)
		kill_mine();
	else 
		if(dorange)
			kill_range(range[0], range[1]);
		else {
			if(!msg_cnt)
				if(append_active_message())
					return error(77);
			kill_messages(TokenList);
		}

	return OK;
}


static void
kill_mine(void)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;

	bzero(&lc, sizeof(lc));
	lc.must_include_mask = MsgRead | MsgMine | MsgActive;
	lc.include_mask = MsgActive;
	lc.exclude_mask = 0;
	build_partial_list(&lc);

	m = MsgDirList;
	while(m) {
		if(m->visible)
			kill_message_number(m);
		NEXT(m);
	}
}

static int
kill_range(int start, int finish)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;
	int num;

	bzero(&lc, sizeof(lc));
	lc.include_mask = MsgActive;
	build_partial_list(&lc);

	for(num=start; num<=finish; num++) {
		m = MsgDirList;
		while(m) {
			if(m->number == num && m->visible) {
				if(IsMsgActive(m)) {
					kill_message_number(m);
					active_message = num;
				}
				break;
			}
			NEXT(m);
		}
	}
	return OK;
}

static void
kill_messages(struct TOKEN *t)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;

	bzero(&lc, sizeof(lc));
	if(ImSysop)
		lc.include_mask = MsgActive | MsgOld | MsgKilled;
	else
		lc.include_mask = MsgActive | MsgOld;
	build_partial_list(&lc);

	while(t->token != END) {
		if(t->token == NUMBER) {
			m = MsgDirList;
			while(m) {
				if(m->number == t->value && m->visible) {
					kill_message_number(m);
					active_message = t->value;
					break;
				}
				NEXT(m);
			}

			if(m == NULL)
				system_msg_number(132, t->value);
		}
		NEXT(t);
	}
}

static int
kill_message_number(struct msg_dir_entry *m)
{
	char *result;
	set_user_list_info();
	set_listing_flags(m);


	if((m->flags & (MsgMine | MsgNTS | MsgSentByMe)) || ImSysop) {
		if(!(m->flags & (MsgActive | MsgOld)) && !ImSysop)
			return error_number(133, m->number);

#if 0 /* NEWDELETE */
		if((m.flags & MsgSentByMe && !(m.flags & MsgSentFromHere)) && !ImSysop) {
		PRINT("Not allowed to delete messages that were not sent from here\n");
		PRINT("Use the HOLD command and send a message to SYSOP explaining\n");
		PRINT("the problem.\n");
			return OK;
		}
#endif
		if(inquire)
			if(!(m->flags & MsgRead) &&
			   !(m->flags & MsgBulletin) &&
			   !(m->flags & MsgNTS)) {
					PRINTF("Message #%d has not been read, kill anyway (N/y)? ", m->number);
					if(get_yes_no(NO) == NO)
						return OK;
			}

		if(hard_delete)
			result = msgd_cmd_num(msgd_xlate(mKILLH), m->number);
		else
			result = msgd_cmd_num(msgd_xlate(mKILL), m->number);

		if(result[0] == 'O' && result[1] == 'K')
			PRINTF("Message #%d Killed\n", m->number);
		else
			PRINTF("Could not kill message #%d\n", m->number);
	} else
		return error(143);

	return OK;
}
