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
#include "msg_immune.h"

static int immune_range(int mode, int start, int finish);
static void immune_messages(int mode, struct TOKEN *t);
static int immune_message_number(int mode, int num);

int
msg_immune_t(int mode, struct TOKEN *t)
{
	int msg_cnt = 0;
	int range[2], dorange = FALSE;

	range[1] = 0;

	while(t->token != END) {
		switch(t->token) {
		case NUMBER:
			if(range[1])
				return bad_cmd(141, t->location);
			msg_cnt++;
			range[dorange] = t->value;
			break;

		case DASH:
			if(!ImSysop)
				return bad_cmd(78, t->location);

			if(range[0] == 0)
				return bad_cmd(140, t->location);
			dorange++;
			break;

		default:
			return bad_cmd(140, t->location);
		}
		NEXT(t);
	}

	if(dorange)
		immune_range(mode, range[0], range[1]);
	else {
		if(!msg_cnt)
			if(append_active_message())
				return error(77);
		immune_messages(mode, TokenList);
	}

	return OK;
}

static int
immune_range(int mode, int start, int finish)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;
	int num;

	bzero(&lc, sizeof(lc));
	lc.include_mask = MsgActive | MsgOld;
	if(build_partial_list(&lc) == 0)
		return OK;

	for(num=start; num<=finish; num++) {
		m = MsgDirList;
		while(m) {
			if(m->number == num && m->visible) {
				if(mode == IMMUNE)
					msgd_cmd_num(msgd_xlate(mIMMUNE), num);
				else
					msgd_cmd_num(msgd_xlate(mACTIVE), num);
				active_message = num;
				break;
			}
			NEXT(m);
		}
	}
	return OK;
}

static void
immune_messages(int mode, struct TOKEN *t)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;

	bzero(&lc, sizeof(lc));
	lc.include_mask = MsgActive | MsgOld;
	build_partial_list(&lc);

	while(t->token != END) {
		if(t->token == NUMBER) {
			m = MsgDirList;
			while(m) {
				if(m->number == t->value && m->visible) {
					if(mode == IMMUNE)
						msgd_cmd_num(msgd_xlate(mIMMUNE), t->value);
					else
						msgd_cmd_num(msgd_xlate(mACTIVE), t->value);
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
immune_message_number(int mode, int num)
{
	switch(mode) {
	case IMMUNE:
		msgd_cmd_num(msgd_xlate(mIMMUNE), num);
		break;
	case REFRESH:
		msgd_cmd_num(msgd_xlate(mACTIVE), num);
		break;
	}

	return OK;
}
