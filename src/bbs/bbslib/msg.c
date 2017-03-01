#include <stdio.h>
#include <string.h>
#ifndef SUNOS
#include <regex.h>
#endif
#include <ctype.h>
#include <unistd.h>
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "rfc822.h"

static long ListMode = mNORMAL;


int
msg_SendMessage(struct msg_dir_entry *m)
{
	char buf[80];
	struct text_line *body, *tl = NULL;
	char *result;

		/* build full message image */

	body = m->body;
	while(body) {
		textline_append(&tl, body->s);
		NEXT(body);
	}

	textline_append(&tl, "/EX");
	rfc822_gen(rFROM, m, buf, 80);
	textline_append(&tl, buf);

	rfc822_gen(rTO, m, buf, 80);
	textline_append(&tl, buf);

	rfc822_gen(rSUBJECT, m, buf, 80);
	textline_append(&tl, buf);

	if(m->flags & MsgBid) {
		rfc822_gen(rBID, m, buf, 80);
		textline_append(&tl, buf);
	}

	rfc822_gen(rCREATE, m, buf, 80);
	textline_append(&tl, buf);
	rfc822_gen(rBORN, m, buf, 80);
	textline_append(&tl, buf);

	if(m->flags & MsgKilled) {
		rfc822_gen(rKILL, m, buf, 80);
		textline_append(&tl, buf);
	}

	if(m->flags & MsgImmune) {
		rfc822_gen(rIMMUNE, m, buf, 80);
		textline_append(&tl, buf);
	}

	rfc822_gen(rTYPE, m, buf, 80);
	textline_append(&tl, buf);
	if(m->flags & MsgPassword) {
		rfc822_gen(rPASSWORD, m, buf, 80);
		textline_append(&tl, buf);
	}


	/* msgd should return message number and bid */

	result = msgd_cmd_textline(msgd_xlate(mSEND), tl);
	textline_free(tl);

	if(isOK(result)) {
		char *p = &result[4];
		m->number = get_number(&p);
		return OK;
	}

	textline_prepend(&error_list, result);
	return ERROR;
}

char *
msg_SetActive(int number)
{
	return msgd_cmd_num(msgd_xlate(mACTIVE), number);
}

char *
msg_SetImmune(int number)
{
	return msgd_cmd_num(msgd_xlate(mIMMUNE), number);
}

char *
msg_SetHeld(int number, struct text_line *reason)
{
	char cmd[80];
	sprintf(cmd, "%s %d", msgd_xlate(mHOLD), number);
	return msgd_cmd_textline(cmd, reason);
}

struct text_line *
msg_WhyHeld(int number)
{
	char cmd[80];
	struct text_line *tl;

	sprintf(cmd, "%s %d", msgd_xlate(mWHY), number);
	if(msgd_fetch_textline(cmd, &tl) != OK) {
		textline_free(tl);
		return NULL;
	}
	return tl;
}

int
msg_ReadBody(struct msg_dir_entry *m)
{
	char cmd[80];

	if(m->body != NULL)
		return OK;

	sprintf(cmd, "%s %ld", msgd_xlate(mREADH), m->number);
	if(msgd_fetch_textline(cmd, &(m->body)) != OK) {
		m->body = textline_free(m->body);
		return ERROR;
	}
	return OK;
}

int
msg_ReadBodyBy(struct msg_dir_entry *m, char *by)
{
	char cmd[80];

	if(m->body != NULL)
		return OK;

	sprintf(cmd, "%s %ld %s", msgd_xlate(mREADH), m->number, by);
	if(msgd_fetch_textline(cmd, &(m->body)) != OK) {
		m->body = textline_free(m->body);
		return ERROR;
	}
	return OK;
}

int
msg_ReadRfc(struct msg_dir_entry *m)
{
	char cmd[80];

	if(m->header != NULL) {
		textline_free(m->header);
		m->header = NULL;
	}

	sprintf(cmd, "%s %ld", msgd_xlate(mREADRFC), m->number);
	if(msgd_fetch_textline(cmd, &(m->header)) != OK) {
		m->header = textline_free(m->header);
		return ERROR;
	}
	return OK;
}

char *
msg_SysopMode(void)
{
	if(ListMode == mSYSOP)
		return "OK\n";
	ListMode = mSYSOP;
	return msgd_cmd(msgd_xlate(mSYSOP));
}

char *
msg_BbsMode(void)
{
	if(ListMode == mBBS)
		return "OK\n";
	ListMode = mBBS;
	return msgd_cmd(msgd_xlate(mBBS));
}

char *
msg_CatchUp(void)
{
	return msgd_cmd(msgd_xlate(mCATCHUP));
}

char *
msg_NormalMode(void)
{
	if(ListMode == mNORMAL)
		return "OK\n";
	ListMode = mNORMAL;
	return msgd_cmd(msgd_xlate(mNORMAL));
}

char *
msg_ListMineMode(void)
{
	if(ListMode == mMINE)
		return "OK\n";
	ListMode = mMINE;
	return msgd_cmd(msgd_xlate(mMINE));
}

char *
msg_ListSinceMode(long t)
{
	char cmd[80];

	if(ListMode == mSINCE)
		return "OK\n";
	ListMode = mSINCE;
	sprintf(cmd, "%s %ld", msgd_xlate(mSINCE), t);
	return msgd_cmd(cmd);
}

long
msg_ListMode(void)
{
	return ListMode;
}

char *
msg_LoginUser(char *call)
{
	char cmd[80];

	uppercase(call);
	sprintf(cmd, "%s %s", msgd_xlate(mUSER), call);
	return msgd_cmd(cmd);
}

static struct text_line
	*our_hloc = NULL;

static void
fetch_our_hloc(void)
{
	char *p, *q, hloc[128];

	strcpy(hloc, bbsd_get_variable("Bbs_Call"));
	textline_append(&our_hloc, hloc);

	strcpy(hloc, bbsd_get_variable("Bbs_Hloc_Comp"));
	uppercase(hloc);
	p = hloc;

	while((q = (char*)strchr(p, '.')) != NULL) {
		*q++ = 0;
		textline_append(&our_hloc, p);
		p = q;
	}

	if(*p)
		textline_append(&our_hloc, p);
}	
 	
int
message_matches_criteria(char *match, struct msg_dir_entry *m, long timenow)
{
	char *field;
	char subject[80];
	char *p, hloc[256];
	char pattern[80];

	while(*match) {
		int found = FALSE;

		switch(*match) {
		case cHLOC:
			if(our_hloc == NULL)
				fetch_our_hloc();

			if(m->to.address[0])
				sprintf(hloc, ".%s.%s", m->to.at.str, m->to.address);
			else
				sprintf(hloc, ".%s", m->to.at.str);
			match++;
			NextChar(match);

			sprintf(pattern, "^%s$", get_string(&match));
			if(re_comp(pattern) != NULL)
				return FALSE;

			/* now let's scan the hlocs */
			while((p = (char*)strrchr(hloc, '.')) != NULL) {
				struct text_line *tl = our_hloc;
			
				*p++ = 0; 
				if(*p == 0)
					return FALSE;

					/* if part of our hloc then simply skip */
				while(tl) {
					if(!strcmp(tl->s, p))
						break;
					NEXT(tl);
				}
				if(tl)
					continue;

				if(re_exec(p) == FALSE)
					return FALSE;
				found = TRUE;
				break;
			}

			if(!found)
				return FALSE;
			continue;

		case cTO:
			field = m->to.name.str;
			break;
		case cFROM:
			field = m->from.name.str;
			break;
		case cAT:
			field = m->to.at.str;
			break;
		case cBID:
			field = m->bid;
			break;
		case cSUBJECT:
			strcpy(subject, m->sub);
			uppercase(subject);
			field = subject;
			break;

		case cAGE_OLDER:
			match++;
			NextChar(match);
			{
				long target =
					timenow - ((long)get_number(&match) * tDay);
				if(m->edate > target)
					return FALSE;
			}
			continue;

		case cAGE_NEWER:
			match++;
			NextChar(match);
			{
				long target =
					timenow - ((long)get_number(&match) * tDay);
				if(m->edate < target)
					return FALSE;
			}
			continue;

		case cFLAG:
			{
				char *t = match;
				int done = FALSE;
				int flag = 0;
				int flg_match = FALSE;

				NextSpace(match);
				NextChar(match);

				t++;
				NextChar(match);
				while(*t) {
					switch(*t) {
					case 'P':
						flag = MsgPersonal; break;
					case 'B':
						flag = MsgBulletin; break;
					case 'T':
						flag = MsgNTS; break;
					case 'S':
						flag = MsgSecure; break;
					case 'W':
						flag = MsgPassword; break;
					case 'L':
						flag = MsgLocal; break;
					case 'C':
						flag = MsgCall; break;
					case 'G':
						flag = MsgCategory; break;
					case 'H':
						flag = MsgSentFromHere; break;
					default:
						done = TRUE;
					}
					if(done) break;

					if(m->flags & flag)
						flg_match = TRUE;

					t++;
				}
				if(flg_match == FALSE)
					return FALSE;
			}
			continue;

		default:
			return FALSE;
		}

		match++;
		NextChar(match);

		sprintf(pattern, "^%s$", get_string(&match));
		if(re_comp(pattern) != NULL)
			return FALSE;

		if(re_exec(field) == FALSE)
			return FALSE;
	}
	return TRUE;
}

