#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#if HAVE_REGCOMP
#include <regex.h>
#endif
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
#include "file.h"
#include "system.h"
#include "msg_fwddir.h"
#include "rfc822.h"
#include "msg_util.h"

static void msg_hash_init(void);
static void msg_hash_set(struct msg_dir_entry *m);
static int msg_hash_get(int num, struct msg_dir_entry **r);
static int matches_criteria(struct msg_dir_entry *ml, struct list_criteria *lc);

int time_list_built = 0;

struct msg_dir_entry
	*free_message_list(void),
	*MsgDirTail = NULL,
	*MsgDirList = NULL;

static struct user_list_info ui;

struct msg_dir_entry *msg_dir_hash[10000];

static void
msg_hash_init(void)
{
	int i;
	for(i=0; i<10000; i++)
		msg_dir_hash[i] = NULL;
}

static void
msg_hash_set(struct msg_dir_entry *m)
{
	int key = m->number % 10000;
	msg_dir_hash[key] = m;
}

static int
msg_hash_get(int num, struct msg_dir_entry **r)
{
	int key = num % 10000;
	struct msg_dir_entry *m = msg_dir_hash[key];

	if(m == NULL) {
		*r = NULL;
		return OK;
	}
	if(m->number == num) {
		*r = m;
		return OK;
	}
	return ERROR;
}

static int
matches_criteria(struct msg_dir_entry *ml, struct list_criteria *lc)
{
	char buf[80];
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */

	if((ml->flags & lc->must_include_mask) != lc->must_include_mask)
		return FALSE;

	if(ml->flags & lc->exclude_mask)
		return FALSE;

	if(lc->range_type == FULL)
		if(ml->number < lc->range[0] || ml->number > lc->range[1])
			return FALSE;

	if(lc->pattern_type & MatchTOmask) {
		if(ImRegExp) {
#if HAVE_REGCOMP
			if (regcomp(&preg, lc->pattern[MatchTO], 0) != 0)
				return FALSE;
			ret = regexec(&preg, ml->to.name.str, 0, NULL, 0);
			regfree(&preg);
			if (ret != 0)
				return FALSE;
#else
			if((re_comp(lc->pattern[MatchTO])) != NULL)
				return FALSE;
			if(re_exec(ml->to.name.str) != 1)
				return FALSE;
#endif /* HAVE_REGCOMP */
		} else {
			if(ml->to.name.sum != lc->pattern_sum[MatchTO])
				return FALSE;
			if(strcmp(ml->to.name.str, lc->pattern[MatchTO]))
				return FALSE;
		}
	}

	if(lc->pattern_type & MatchFROMmask) {
		if(ImRegExp) {
#if HAVE_REGCOMP
			if (regcomp(&preg, lc->pattern[MatchFROM], 0) != 0)
				return FALSE;
			ret = regexec(&preg, ml->from.name.str, 0, NULL, 0);
			regfree(&preg);
			if (ret != 0)
				return FALSE;
#else
			if((re_comp(lc->pattern[MatchFROM])) != NULL)
				return FALSE;
			if(re_exec(ml->from.name.str) != 1)
				return FALSE;
#endif /* HAVE_REGCOMP */
		} else {
			if(ml->from.name.sum != lc->pattern_sum[MatchFROM])
				return FALSE;
			if(strcmp(ml->from.name.str, lc->pattern[MatchFROM]))
				return FALSE;
		}
	}

	if(lc->pattern_type & MatchATmask) {
		if(ImRegExp) {
#if HAVE_REGCOMP
			if (regcomp(&preg, lc->pattern[MatchAT], 0) != 0)
				return FALSE;
			ret = regexec(&preg, ml->to.at.str, 0, NULL, 0);
			regfree(&preg);
			if (ret != 0)
				return FALSE;
#else
			if((re_comp(lc->pattern[MatchAT])) != NULL)
				return FALSE;
			if(re_exec(ml->to.at.str) != 1)
				return FALSE;
#endif /* HAVE_REGCOMP */
		} else {
			if(ml->to.at.sum != lc->pattern_sum[MatchAT])
				return FALSE;
			if(strcmp(ml->to.at.str, lc->pattern[MatchAT]))
				return FALSE;
		}
	}

	if(lc->pattern_type & MatchSUBmask) {
		case_strcpy(buf, ml->sub, AllUpperCase);
		if(ImRegExp) {
#if HAVE_REGCOMP
			if (regcomp(&preg, lc->pattern[MatchSUB], 0) != 0)
				return FALSE;
			ret = regexec(&preg, buf, 0, NULL, 0);
			regfree(&preg);
			if (ret != 0)
				return FALSE;
#else
			if((re_comp(lc->pattern[MatchSUB])) != NULL)
				return FALSE;
			if(re_exec(buf) != 1)
				return FALSE;
#endif /* HAVE_REGCOMP */
		} else
			if(strstr(buf, lc->pattern[MatchSUB]) == NULL)
				return FALSE;
	}

	if(lc->since != 0) {
		if(ml->cdate < lc->since)
			return FALSE;
	}
	return TRUE;
}

void
set_user_list_info(void)
{
	bzero(&ui, sizeof(ui));

	ui.number = user_get_value(uNUMBER);
	ui.last_msg = user_get_value(uMESSAGE);
	strcpy(ui.call.str, usercall);
	ui.call.sum = sum_string(ui.call.str);

	user_get_list(uINCLUDE);
	user_get_list(uEXCLUDE);
}

void
SetMsgActive(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= MsgActive;
}

void
SetMsgOld(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= (MsgOld | MsgActive);
}

void
SetMsgHeld(struct msg_dir_entry *m)
{
		/* caller should handle rfc822 generation */
	m->flags &= ~MsgStatusMask;
	m->flags |= (MsgHeld | MsgActive);
}

void
SetMsgKilled(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= MsgKilled;
}

void
SetMsgImmune(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= (MsgActive | MsgImmune);
}

void
ClrMsgImmune(struct msg_dir_entry *m)
{
	if(m->flags & MsgImmune)
		m->flags &= ~MsgImmune;
}

void
SetMsgPersonal(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgPersonal;
}

void
SetMsgBulletin(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgBulletin;
}

void
SetMsgNTS(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgNTS;
}

void
SetMsgSecure(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgSecure;
	m->flags |= MsgPersonal;
}

void
SetMsgPassword(struct msg_dir_entry *m, char *s)
{
	SetMsgSecure(m);
	m->flags |= MsgPassword;
	strcpy(m->passwd, s);
}

void
SetMsgNoFwd(struct msg_dir_entry *m)
{
	m->flags &= ~MsgPending;
	m->flags |= MsgNoFwd;
}

void
SetMsgLocal(struct msg_dir_entry *m)
{
	m->flags &= ~MsgKindMask;
	m->flags |= MsgLocal;
}

void
SetMsgCall(struct msg_dir_entry *m)
{
	m->flags &= ~MsgKindMask;
	m->flags |= MsgCall;
}

void
SetMsgCategory(struct msg_dir_entry *m)
{
	m->flags &= ~MsgKindMask;
	m->flags |= MsgCategory;
}

void
flush_message_list(void)
{
	msg_cmd(mFLUSH);
	free_message_list();
}

void
msg_forwarded(int num, char *alias)
{
	char cmd[80];

	sprintf(cmd, "%s %d", msgd_xlate(mFORWARD), num);
	if(alias)
		sprintf(cmd, "%s %s", cmd, alias);

	msgd_cmd(cmd);
}

static int LineCount;

/*ARGSUSED*/
void
line_counter(char *s)
{
	LineCount++;
}

int
msg_pending_cnt(int num)
{
	char cmd[80];
	sprintf(cmd, "%s %d", msgd_xlate(mPENDING), num);
	LineCount = 0;
	msgd_fetch_multi(cmd, line_counter);
	return LineCount;
}

int
msg_edit_issue(int num, char *rfc)
{
	char cmd[256];
	char *c;

	sprintf(cmd, "%s %d %s", msgd_xlate(mEDIT), num, rfc);
	c = msgd_cmd(cmd);

	if(c[0] == 'O' && c[1] == 'K')
		return OK;
	return ERROR;
}

int
msg_cmd(int token)
{
	char *c;

	c = msgd_cmd(msgd_xlate(token));
	
	if(c[0] == 'O' && c[1] == 'K')
		return OK;
	return ERROR;
}

static struct msg_dir_entry *
msg_quick_locate(int msgnum)
{
	struct msg_dir_entry *m;

	if (msg_hash_get(msgnum, &m) == OK)
		return m;
	
	m = MsgDirList;
	while(m) {
		if(m->number == msgnum)
			return m;
		NEXT(m);
	}
	return NULL;
}

struct msg_dir_entry *
msg_locate(int msgnum)
{
	struct msg_dir_entry *m;

	if (msg_hash_get(msgnum, &m) == OK)
		return m;
	
	m = MsgDirList;
	while(m) {
		if(m->number == msgnum)
			return m;
		NEXT(m);
	}

		/* in an attempt to speed accessing msgd we have reduced the
		 * number of times we update our internal tables. We should
		 * therefore get a fresh copy if the message isn't here.
		 */
	build_full_message_list();
	m = MsgDirList;
	while(m) {
		if(m->number == msgnum)
			return m;
		NEXT(m);
	}
	return NULL;
}

static int
msg_delete(int msgnum)
{
	struct msg_dir_entry *m = msg_quick_locate(msgnum);

	if(m == NULL)
		return ERROR;

	if(m->last != NULL)
		m->last->next = m->next;
	else
		MsgDirList = m->next;

	if(m->next != NULL)
		m->next->last = m->last;

	free(m);
	return OK;
}

static int
msg_parse(struct msg_dir_entry *m, char *s, int read_by_me)
{
	char *q, *p = s;

	m->number = get_number(&p);
	m->size = get_number(&p);
	m->flags = get_number(&p) | read_by_me;

	if(*p == '!') {
		p++;
		strcpy(m->passwd, get_string(&p));
		m->flags |= MsgPassword;
	}

	if(*p != '$') {
		msg_delete(m->number);
		return ERROR;
	}

	p++;
	if(!isspace(*p)) {
		strcpy(m->bid, get_string(&p));
		m->flags |= MsgBid;
	} else {
		m->bid[0] = 0;
		NextChar(p);
	}

	if((q = get_string_to(&p, '@')) == NULL) {
		msg_delete(m->number);
		return ERROR;
	}
	if(*p != '@') {
		msg_delete(m->number);
		return ERROR;
	}

	strcpy(m->to.name.str, q);
	p++;

	if(!isspace(*p))
		strcpy(m->to.at.str, get_string(&p));
	else
		NextChar(p);

	strcpy(m->from.name.str, get_string(&p));

	m->cdate = get_number(&p);
	m->read_cnt = get_number(&p);
	strcpy(m->sub, p);

	m->to.name.sum = sum_string(m->to.name.str);
	m->to.at.sum = sum_string(m->to.at.str);
	m->from.name.sum = sum_string(m->from.name.str);

	set_listing_flags(m);
	msg_hash_set(m);
	return OK;
}

static int
msg_insert(struct msg_dir_entry *m, int num)
{
	struct msg_dir_entry *msg = MsgDirList;

	if(MsgDirList == NULL) {
		MsgDirList = m;
		return OK;
	}

	if(MsgDirList->number > num) {
		m->next = MsgDirList;
		MsgDirList = m;
		return OK;
	}

	while(msg->next) {
		if(msg->next->number > num) {
			m->next = msg->next;
			m->next->last = m;
			msg->next = m;
			m->last = msg;
			return OK;
		}
		NEXT(msg);
	}

	msg->next = m;
	m->last = msg;
	return OK;
}

static void
msg_add(char *s)
{
	int msgnum, mread = FALSE;
	struct msg_dir_entry *m;

	switch(*s) {
	case '-':
		PRINT("msg_add(-) shouldn't be getting here now\n");
		s+=2;
		msgnum = atoi(s);
		msg_delete(msgnum);
		return;
	case '+':
		PRINT("msg_add(+) shouldn't be getting here now\n");
		s+=2;
		msgnum = atoi(s);
		m = malloc_struct(msg_dir_entry);
		if(msg_insert(m, msgnum) == ERROR)
			return;
		break;

	case 'R':
		mread = MsgReadByMe;
		s++;

	default:
#ifdef CHG_LIST
		msgnum = atoi(s);
		m = malloc_struct(msg_dir_entry);
		msg_append(m);
#else
		msgnum = atoi(s);
		if((m = msg_quick_locate(msgnum)) == NULL) {
			m = malloc_struct(msg_dir_entry);
			if(msg_insert(m, msgnum) == ERROR)
				return;
		}
#endif
		break;
		
	case '*':
		PRINT("msg_add(*) shouldn't be getting here now\n");
		s+=2;
		msgnum = atoi(s);
		if((m = msg_quick_locate(msgnum)) == NULL)
			return;
		break;
	case '^':
		PRINT("msg_add(^) shouldn't be getting here now\n");
		s+=2;
		msgnum = atoi(s);
		if((m = msg_quick_locate(msgnum)) == NULL)
			return;
		break;
	}

	msg_parse(m, s, mread);
}

static struct text_line *msg_buffer = NULL;

static void
msg_fetch(char *s)
{
	textline_append(&msg_buffer, s);
}

struct msg_dir_entry *
build_full_message_list(void)
{
	struct text_line *mb;
	set_user_list_info();
	msgd_fetch_multi(msgd_xlate(mLIST), msg_fetch);
	mb = msg_buffer;
	while(mb) {
		msg_add(mb->s);
		NEXT(mb);
	}
	msg_buffer = textline_free(msg_buffer);
	return MsgDirList;
}

struct msg_dir_entry *
free_message_list(void)
{
	struct msg_dir_entry *tmp;
	
	while(MsgDirList) {
		tmp = MsgDirList;
		NEXT(MsgDirList);
		if(tmp->body)
			msg_free_body(tmp);
		free(tmp);
	}
	MsgDirTail = NULL;
	msg_hash_init();
	return NULL;
}

int
set_listing_flags(struct msg_dir_entry *m)
{
	struct IncludeList *list;

	m->flags &= (MsgWriteMask | MsgBid);

	if(!(m->flags & MsgReadByMe))
		m->flags |= MsgNotReadByMe;

	if(!(m->flags & MsgRead))
		m->flags |= MsgNotRead;

	if(m->to.name.sum == ui.call.sum)
		if(!strcmp(m->to.name.str, ui.call.str))
			m->flags |= MsgMine;
	if(m->from.name.sum == ui.call.sum)
		if(!strcmp(m->from.name.str, ui.call.str))
			m->flags |= MsgSentByMe;

	if(m->flags & MsgPersonal && !ImSysop && !(m->flags & (MsgMine | MsgSentByMe)))
		return ERROR;

	if(m->to.at.str[0] == 0)
		m->flags |= MsgLocal;
	else
		if(m->to.at.sum == bbscallsum)
			if(!strcmp(m->to.at.str, Bbs_Call))
				m->flags |= MsgLocal;

	list = Include;
	while(list) {
		struct IncludeList *t = list;
		int match = TRUE;
		while(t && match) {
			char *s;
			switch(t->key) {
			case '>':
				s = m->to.name.str;
				break;
			case '<':
				s = m->from.name.str;
				break;
			case '@':
				s = m->to.at.str;
				break;
			}
			if(strcmp(s, t->str))
				match = FALSE;
			t = t->comp;
		}

		if(match) {
			m->flags |= MsgClub;
			break;
		}
		NEXT(list);
	}

	m->flags |= MsgSkip;

	list = Exclude;
	while(list) {
		struct IncludeList *t = list;
		int match = TRUE;
		while(t && match) {
			char *s;
			switch(t->key) {
			case '>':
				s = m->to.name.str;
				break;
			case '<':
				s = m->from.name.str;
				break;
			case '@':
				s = m->to.at.str;
				break;
			}
			if(strcmp(s, t->str))
				match = FALSE;
			t = t->comp;
		}

		if(match) {
			m->flags &= ~MsgSkip;
			break;
		}
		NEXT(list);
	}

	if(m->cdate > ui.last_msg)
		m->flags |= MsgNew;

	return OK;
}

int
build_partial_list(struct list_criteria *lc)
{
	int cnt = 0;
	struct msg_dir_entry *ml =  build_full_message_list();

	while(ml) {
		ml->visible = FALSE;
		NEXT(ml);
	}
	ml = MsgDirList;

	while(ml) {
		if(matches_criteria(ml, lc)) {
			ml->visible = TRUE;
			cnt++;
			if(lc->range_type == FIRST && cnt == lc->range[0])
				break;
		}
		NEXT(ml);	
	}

	if(lc->range_type == LAST && cnt > lc->range[0])
		return lc->range[0];
	return cnt;
}

