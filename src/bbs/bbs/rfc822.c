#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "vars.h"
#include "rfc822.h"

#ifdef NOMSGD
int
rfc822_append_tl(int num, int token, struct text_line *tl)
{
	FILE *fp = open_message(num);

	if(fp == NULL)
		return ERROR;

	fseek(fp, 0, 2);
	while(tl) {
		fprintf(fp, "%s %s\n", rfc822_xlate(token), tl->s);
		NEXT(tl);
	}
	close_message(fp);
}

int
rfc822_append(int num, int token, char *s)
{
	FILE *fp = open_message(num);
	if(fp == NULL)
		return ERROR;

	fseek(fp, 0, 2);
	fprintf(fp, "%s %s\n", rfc822_xlate(token), s);
	close_message(fp);
}

int
rfc822_display_held(int num)
{
	FILE *fp = open_message(num);
	char buf[1024];
	int cnt = 0;
	int heldby = strlen(rfc822_xlate(rHELDBY));
	int heldwhy = strlen(rfc822_xlate(rHELDWHY));

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);
	while(fgets(buf, 1024, fp)) {
		if(!strncmp(buf, rfc822_xlate(rHELDBY), heldby)) {
			cnt++;
			PRINTF("  By: %s", &buf[strlen(rfc822_xlate(rHELDBY))+1]);
			continue;
		}
		if(!strncmp(buf, rfc822_xlate(rHELDWHY), heldwhy)) {
			PRINTF("      %s", &buf[strlen(rfc822_xlate(rHELDWHY))+1]);
			continue;
		}
	}
	close_message(fp);
	return cnt;
}

int
rfc822_display_field(int num, int token)
{
	FILE *fp = open_message(num);
	char buf[1024];
	int len = strlen(rfc822_xlate(token));

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);
	while(fgets(buf, 1024, fp))
		if(!strncmp(buf, rfc822_xlate(token), len))
			PRINTF("%s", buf);

	close_message(fp);
}

char *
rfc822_get_field(int num, int token)
{
	FILE *fp = open_message(num);
	char buf[1024];
	static char output[1024];
	int len = strlen(rfc822_xlate(token));

	if(fp == NULL)
		return NULL;

	rfc822_skip_to(fp);
	while(fgets(buf, 1024, fp))
		if(!strncmp(buf, rfc822_xlate(token), len))
			strcpy(output, &buf[strlen(rfc822_xlate(token))+1]);

	close_message(fp);

	output[strlen(output)-1] = 0;
	return output;
}

int
rfc822_skip_to(FILE *fp)
{
	char buf[1024];

	while(fgets(buf, 1024, fp))
		if(!strcmp(buf, "/EX\n"))
			return OK;
	return ERROR;
}

int
rfc822_genx(FILE *fp, struct message_directory_entry *m)
{
	char type = 'U';

	fprintf(fp, "%s %s", rfc822_xlate(rFROM), m->from.name.str);
	if(m->from.at.str[0]) {
		fprintf(fp, "@%s", m->from.at.str);
		if(m->from.address[0])
			fprintf(fp, ".%s", m->from.address);
	}
	fprintf(fp, "\n");

	fprintf(fp, "%s %s", rfc822_xlate(rTO), m->to.name.str);
	if(m->to.at.str[0]) {
		fprintf(fp, "@%s", m->to.at.str);
		if(m->to.address[0])
			fprintf(fp, ".%s", m->to.address);
	}
	fprintf(fp, "\n");

	fprintf(fp, "%s %s\n", rfc822_xlate(rSUBJECT), m->sub);
	if(m->bid[0])
		fprintf(fp, "%s %s\n", rfc822_xlate(rBID), m->bid);
	
	fprintf(fp, "%s %d\n", rfc822_xlate(rCREATE), m->cdate);

	switch(m->flags & MsgTypeMask) {
	case MsgPersonal:	type = 'P'; break;
	case MsgBulletin:	type = 'B'; break;
	case MsgNTS:		type = 'T'; break;
	case MsgSecure:		type = 'S'; break;
	}
	fprintf(fp, "%s %c\n", rfc822_xlate(rTYPE), type);
	if(m->flags & MsgPassword)
		fprintf(fp, "%s %s\n", rfc822_xlate(rPASSWORD), m->passwd);
	return OK;
}

int
rfc822_create(struct message_directory_entry *m)
{
	int result = ERROR;
	char fn[256];
	FILE *fp;

	sprintf(fn, "%s/%05d", MSGBODYPATH, m->number);
	if((fp = fopen(fn, "a")) == NULL)
		return ERROR;

	result = rfc822_genx(fp, m);
	fclose(fp);
	return result;
}

int
rfc822_decode_fields(struct message_directory_entry *m)
{
	FILE *fp = open_message(m->number);
	char buf[1024];
	static char output[1024];
	int nothing = TRUE;

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);
	m->size = ftell(fp);
	while(fgets(buf, 1024, fp)) {
		buf[strlen(buf)-1] = 0;

		nothing = FALSE;
		if(!strncmp(buf, "X-Bid:", 6)) {
			strcpy(m->bid, &buf[7]);
			continue;
		}
		if(!strncmp(buf, "X-Create:", 9)) {
			m->cdate = atoi(&buf[10]);
			m->flags &= ~MsgStatusMask;
			m->flags |= MsgActive;
			continue;
		}
		if(!strncmp(buf, "X-Kill:", 7)) {
			m->kdate = atoi(&buf[8]);
			m->flags &= ~MsgStatusMask;
			m->flags |= MsgKilled;
			continue;
		}
		if(!strncmp(buf, "X-Old:", 6)) {
			m->odate = atoi(&buf[7]);
			m->flags &= ~MsgStatusMask;
			m->flags |= (MsgOld | MsgActive);
			continue;
		}

		if(!strncmp(buf, "X-HeldBy:", 9)) {
			m->flags &= ~MsgStatusMask;
			m->flags |= (MsgHeld | MsgActive);
			continue;
		}
		if(!strncmp(buf, "X-HeldReason:", 13)) {
			continue;
		}

		if(!strncmp(buf, "X-Immune:", 9)) {
			if(!strncmp(&buf[10], "ON", 2))
				m->flags |= MsgImmune;
			else
				m->flags &= ~MsgImmune;
			continue;
		}
		if(!strncmp(buf, "X-Password:", 11)) {
			strcpy(m->passwd, &buf[12]);
			continue;
		}
		if(!strncmp(buf, "Subject:", 8)) {
			strcpy(m->sub, &buf[9]);
			continue;
		}
		if(!strncmp(buf, "To:", 3)) {
			char *p = &buf[4];
			NextChar(p);
			if(*p) {
				strcpy(m->to.name.str, get_call(&p));
				m->to.name.sum = sum_string(m->to.name.str);
				if(*p == '@') {
					p++;
					strcpy(m->to.at.str, get_call(&p));
					m->to.at.sum = sum_string(m->to.at.str);
					if(*p == '.') {
						p++;
						strcpy(m->to.address, get_string(&p));
					}
				}
			}
		}
		if(!strncmp(buf, "From:", 5)) {
			char *p = &buf[6];
			NextChar(p);
			if(*p) {
				strcpy(m->from.name.str, get_call(&p));
				m->from.name.sum = sum_string(m->from.name.str);
				if(*p == '@') {
					p++;
					strcpy(m->from.at.str, get_call(&p));
					m->from.at.sum = sum_string(m->from.at.str);
					if(*p == '.') {
						p++;
						strcpy(m->from.address, get_string(&p));
					}
				}
			}
		}
		if(!strncmp(buf, "X-Type:", 7)) {
			char *p = &buf[7];
			NextChar(p);
			m->flags &= ~MsgTypeMask;

			switch(*p) {
			case 'P':
				m->flags |= MsgPersonal; break;
			case 'B':
				m->flags |= MsgBulletin; break;
			case 'T':
				m->flags |= MsgNTS; break;
			case 'S':
				m->flags |= MsgSecure; break;
			default:
				nothing = TRUE;
			}
			if(nothing)
				break;
		}
#if 1
		if(!strncmp(buf, "X-ReadBy:", 9)) 
			if(m->read_cnt < MAX_READ_BY) {
				user_focus(&buf[10]);
				m->read_by[m->read_cnt++] = user_get_value(uNUMBER);
				user_cmd(uFLUSH);
			}
#endif

		if(!strncmp(buf, "X-NewsGroup:", 12)) {
			if(m->grp_cnt < MAX_GROUPS)
				strcpy(m->groups[m->grp_cnt++], &buf[13]);
		}

		if(!strncmp(buf, "X-ReadByRcpt:", 13)) {
			if(m->read_cnt < MAX_READ_BY) {
				long unum = user_get_value(uNUMBER);
				if(unum != ERROR)
					m->read_by[m->read_cnt++] = (short)unum;
			}
			m->flags |= MsgRead;
		}
	}
	close_message(fp);

	if(nothing)
		return ERROR;
	return OK;
	
}
#endif
