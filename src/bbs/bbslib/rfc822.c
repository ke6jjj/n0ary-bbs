#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "rfc822.h"

static void
	rfc822_gen_bid(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_create(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_born(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_from(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_heldby(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_heldreason(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_immune(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_kill(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_time2live(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_password(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_readby(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_replyto(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_subject(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_to(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_gen_type(struct RfcFields *rf, struct msg_dir_entry *m, char *buf),
	rfc822_parse_bid(struct msg_dir_entry *m, char *buf),
	rfc822_parse_create(struct msg_dir_entry *m, char *buf),
	rfc822_parse_born(struct msg_dir_entry *m, char *buf),
	rfc822_parse_from(struct msg_dir_entry *m, char *buf),
	rfc822_parse_heldby(struct msg_dir_entry *m, char *buf),
	rfc822_parse_heldreason(struct msg_dir_entry *m, char *buf),
	rfc822_parse_immune(struct msg_dir_entry *m, char *buf),
	rfc822_parse_kill(struct msg_dir_entry *m, char *buf),
	rfc822_parse_time2live(struct msg_dir_entry *m, char *buf),
	rfc822_parse_password(struct msg_dir_entry *m, char *buf),
	rfc822_parse_readby(struct msg_dir_entry *m, char *buf),
	rfc822_parse_replyto(struct msg_dir_entry *m, char *buf),
	rfc822_parse_subject(struct msg_dir_entry *m, char *buf),
	rfc822_parse_to(struct msg_dir_entry *m, char *buf),
	rfc822_parse_type(struct msg_dir_entry *m, char *buf);

struct RfcFields RfcFlds[] = {
	{ rBID,		"X-Bid:",			rfc822_gen_bid,		rfc822_parse_bid },
	{ rBORN,	"X-Born:",			rfc822_gen_born,	rfc822_parse_born },
	{ rCREATE,	"X-Create:",		rfc822_gen_create,	rfc822_parse_create },
	{ rFROM,	"From:",			rfc822_gen_from,	rfc822_parse_from },
	{ rHELDBY,	"X-HeldBy:",		rfc822_gen_heldby,	rfc822_parse_heldby },
	{ rHELDWHY,	"X-HeldReason:",	rfc822_gen_heldreason,	rfc822_parse_heldreason },
	{ rIMMUNE,	"X-Immune:",		rfc822_gen_immune,	rfc822_parse_immune },
	{ rKILL,	"X-Kill:",			rfc822_gen_kill,	rfc822_parse_kill },
	{ rLIVE,	"X-Time2Live:",		rfc822_gen_time2live,	rfc822_parse_time2live },
	{ rPASSWORD,"X-Password:",		rfc822_gen_password,	rfc822_parse_password },
	{ rREADBY,	"X-ReadBy:",		rfc822_gen_readby,	rfc822_parse_readby },
	{ rREPLY,	"ReplyTo:",			rfc822_gen_replyto,	rfc822_parse_replyto },
	{ rSUBJECT,	"Subject:",			rfc822_gen_subject,	rfc822_parse_subject },
	{ rTO,		"To:",				rfc822_gen_to,		rfc822_parse_to },
	{ rTYPE,	"X-Type:",			rfc822_gen_type,	rfc822_parse_type },
	{ ERROR,	NULL,				NULL, NULL}};

char *
rfc822_xlate(int field)
{
	struct RfcFields *rf = RfcFlds;
	while(rf->token != ERROR) {
		if(rf->token == field)
			return rf->text;
		rf++;
	}
	return NULL;
}

int
rfc822_gen(int field, struct msg_dir_entry *m, char *out, int len)
{
	char buf[1024];
	struct RfcFields *rf = RfcFlds;

	while(rf->token != ERROR) {
		if(rf->token == field) {
			rf->encode(rf, m, buf);
			buf[len-1] = 0;
			strcpy(out, buf);
			return OK;
		}
		rf++;
	}
	*out = 0;
	return ERROR;
}

int
rfc822_parse(struct msg_dir_entry *m, char *buf)
{
	struct RfcFields *rf = RfcFlds;

	if(buf == NULL)
		return ERROR;

	while(rf->token != ERROR) {
		int len = strlen(rf->text);
		if(!strncmp(rf->text, buf, len)) {
			if(m != NULL) {
				char *p = &buf[len];
				NextChar(p);
				rf->decode(m, p);
			}
			return rf->token;
		}
		rf++;
	}
	return ERROR;
}

char *
rfc822_find(int token, struct text_line *tl)
{
	struct RfcFields *rf = RfcFlds;
	char *str = NULL;

		/* begin by locating desired token in table */
	while(rf->token != token) {
		rf++;
		if(rf->token == ERROR)
			return NULL;
	}

		/* now scan through tl looking for a match, remember to
		 * not stop when you hit the first one. We override an entry
		 * by just appending the new one.
		 */

	while(tl) {
		if(!strncmp(rf->text, tl->s, strlen(rf->text)))
			str = tl->s;
		NEXT(tl);
	}
	return str;
}

static void
rfc822_gen_bid(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %s", rf->text, m->bid);
}

static void
rfc822_gen_create(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %"PRId64, rf->text, m->cdate);
}

static void
rfc822_gen_born(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %"PRId64, rf->text, m->edate);
}

static void
rfc822_gen_from(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %s", rf->text, m->from.name.str);
	if(m->from.at.str[0]) {
		sprintf(buf, "%s@%s", buf, m->from.at.str);
		if(m->from.address[0])
			sprintf(buf, "%s.%s", buf, m->from.address);
	}
}

/*ARGSUSED*/
static void
rfc822_gen_heldby(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
}

/*ARGSUSED*/
static void
rfc822_gen_heldreason(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
}

/*ARGSUSED*/
static void
rfc822_gen_immune(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s", rf->text);
}

static void
rfc822_gen_kill(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %"PRId64, rf->text, m->kdate);
}

static void
rfc822_gen_time2live(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %"PRId64, rf->text, m->time2live);
}

static void
rfc822_gen_password(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %s", rf->text, m->passwd);
}

/*ARGSUSED*/
static void
rfc822_gen_readby(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
}

/*ARGSUSED*/
static void
rfc822_gen_replyto(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %s", rf->text, m->from.address);
}

static void
rfc822_gen_subject(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %s", rf->text, m->sub);
}

static void
rfc822_gen_to(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	sprintf(buf, "%s %s", rf->text, m->to.name.str);
	if(m->to.at.str[0]) {
		sprintf(buf, "%s@%s", buf, m->to.at.str);
		if(m->to.address[0])
			sprintf(buf, "%s.%s", buf, m->to.address);
	}
}

static void
rfc822_gen_type(struct RfcFields *rf, struct msg_dir_entry *m, char *buf)
{
	char c;

	switch(m->flags & MsgTypeMask) {
	case MsgPersonal:	c = 'P'; break;
	case MsgBulletin:	c = 'B'; break;
	case MsgNTS:	c = 'T'; break;
	case MsgSecure:	c = 'S'; break;
	default:	c = 'U'; break;
	}

	sprintf(buf, "%s %c", rf->text, c);
}

static long
xlate_date(char *s)
{
	char *c = (char*)index(s, '/');
	long t;
	if(c == NULL)
		return atol(s);

	t = str2time_t(s);
	c = (char*)index(s, '\n');
	sprintf(s, "%ld", t);
	if(c != NULL)
		strcat(s, "\n");
	return t;
}

static void
rfc822_parse_bid(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	strncpy(m->bid, buf, LenBID);
}

static void
rfc822_parse_create(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	m->cdate = xlate_date(buf);
	m->flags &= ~MsgStatusMask;
	m->flags |= MsgActive;

	if(m->edate == 0)
		m->edate = m->cdate;
}

static void
rfc822_parse_born(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	m->edate = xlate_date(buf);
}

static void
rfc822_parse_from(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	uppercase(buf);
	if(*buf) {
		strcpy(m->from.name.str, get_call(&buf));
		m->from.name.sum = sum_string(m->from.name.str);
		if(*buf == '@') {
			buf++;
			strcpy(m->from.at.str, get_call(&buf));
			m->from.at.sum = sum_string(m->from.at.str);
			if(*buf == '.') {
				buf++;
				strcpy(m->from.address, get_string(&buf));
			}
		}
	}
}

/*ARGSUSED*/
static void
rfc822_parse_heldby(struct msg_dir_entry *m, char *buf)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= MsgHeld;
}

/*ARGSUSED*/
static void
rfc822_parse_heldreason(struct msg_dir_entry *m, char *buf)
{
}

/*ARGSUSED*/
static void
rfc822_parse_immune(struct msg_dir_entry *m, char *buf)
{
	m->flags |= MsgImmune;
}

static void
rfc822_parse_kill(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	m->kdate = xlate_date(buf);
	m->flags &= ~MsgStatusMask;
	m->flags |= MsgKilled;
}

static void
rfc822_parse_time2live(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	m->time2live = xlate_date(buf);
}

static void
rfc822_parse_password(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	strncpy(m->passwd, buf, LenPASSWD);
}

static void
rfc822_parse_readby(struct msg_dir_entry *m, char *buf)
{
	struct text_line *tl = m->read_by;
	if(buf == NULL)
		return;
	while(tl) {
		if(!strcmp(tl->s, buf))
			return;
		NEXT(tl);
	}

	m->read_cnt++;
	textline_prepend(&(m->read_by), buf);
}

/*ARGSUSED*/
static void
rfc822_parse_replyto(struct msg_dir_entry *m, char *buf)
{
}

static void
rfc822_parse_subject(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	strncpy(m->sub, buf, LenSUBJECT);
}

static void
rfc822_parse_to(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	uppercase(buf);
	if(*buf) {
		char *s = m->to.name.str;

#if 1
		while((*buf != '@') && *buf)
			*s++ = *buf++;
		*s = 0;
#else
		strcpy(m->to.name.str, get_call(&buf));
#endif
		m->to.name.sum = sum_string(m->to.name.str);
		if(*buf == '@') {
			buf++;
			if(*buf == 0)
				return;
			strcpy(m->to.at.str, get_call(&buf));
			m->to.at.sum = sum_string(m->to.at.str);
			if(*buf == '.') {
				buf++;
				if(*buf == 0)
					return;
				strcpy(m->to.address, get_string(&buf));
			}
		}
	}
}

static void
rfc822_parse_type(struct msg_dir_entry *m, char *buf)
{
	if(buf == NULL)
		return;
	uppercase(buf);
	m->flags &= ~MsgTypeMask;

	switch(*buf) {
	case 'P':
		m->flags |= MsgPersonal; break;
	case 'B':
		m->flags |= MsgBulletin; break;
	case 'T':
		m->flags |= MsgNTS; break;
	case 'S':
		m->flags |= MsgSecure; break;
	}
}

