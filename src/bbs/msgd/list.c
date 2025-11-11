#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "rfc822.h"
#include "msgd.h"

TAILQ_HEAD(MsgList, msg_dir_entry) MsgDir = TAILQ_HEAD_INITIALIZER(MsgDir);

struct groups
	*GrpList;

void
append_group(char *name, struct msg_dir_entry *msg)
{
	struct groups **grp = &GrpList;
	struct message_list **ml, *m = malloc_struct(message_list);

	uppercase(name);
	m->msg = msg;

	while(*grp) {
		if(!strcmp((*grp)->name, name))
			break;
		grp = &((*grp)->next);
	}

	if(*grp == NULL) {
		*grp = malloc_struct(groups);
		strlcpy((*grp)->name, name, sizeof((*grp)->name));
	}

	if(msg->flags & (MsgActive))
		(*grp)->cnt++;
	ml = &((*grp)->list);

	while(*ml) {
		if((*ml)->next)
			if(m->msg->number < (*ml)->msg->number) {
				m->next = *ml;
				break;
			}
		ml = &((*ml)->next);
	}
	*ml = m;
}

void
show_groups(struct active_processes *ap)
{
	struct groups *grp = GrpList;

	while(grp) {
		char buf[80];
		snprintf(buf, sizeof(buf), "%s\t%d\n", grp->name, grp->cnt);
		socket_raw_write(ap->fd, buf);
		NEXT(grp);
	}
}

void
remove_from_groups(struct msg_dir_entry *msg)
{
	struct groups *grp = GrpList;

	while(grp) {
		struct message_list **ml = &(grp->list);
		while(*ml) {
			if((*ml)->msg == msg) {
				struct message_list *m = *ml;
				*ml = (*ml)->next;
				free(m);
				grp->cnt--;
				break;
			}
			ml = &((*ml)->next);
		}
		NEXT(grp);
	}
}

int
set_groups(struct msg_dir_entry *m)
{
	FILE *fp;
	int found = FALSE;
	char buf[256];
	time_t now = Time(NULL);

	if((fp = fopen(Msgd_Group_File, "r")) == NULL)
		return FALSE;

	while(fgets(buf, sizeof(buf), fp)) {
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
			found = TRUE;
			while(*replace && *replace != ';') {
				char *s = get_string(&replace);
				append_group(s, m);
			}
		}
	}

	if(!found) {
		append_group("MISC", m);
	}

	fclose(fp);
	return TRUE;
}

struct groups *
find_group(char *name)
{
	struct groups *grp = GrpList;

	uppercase(name);
	if(!strcmp(name, "OFF"))
		return NULL;

	while(grp) {
		if(!strcmp(name, grp->name))
			return grp;
		NEXT(grp);
	}
	return NULL;
}

struct msg_dir_entry *
append_msg_list(void)
{
	struct msg_dir_entry *m = malloc_struct(msg_dir_entry);

	TAILQ_INSERT_TAIL(&MsgDir, m, entries);

	return m;
}

struct msg_dir_entry *
unlink_msg_list(struct msg_dir_entry *msg)
{
	struct msg_dir_entry *prev = TAILQ_PREV(msg, MsgList, entries);
	TAILQ_REMOVE(&MsgDir, msg, entries);

	if(msg->read_by != NULL)
		textline_free(msg->read_by);
	free(msg);

	return prev;
}

void
free_msgdir(void)
{
	struct msg_dir_entry *m;

	while (!TAILQ_EMPTY(&MsgDir)) {
		m = TAILQ_FIRST(&MsgDir);
		TAILQ_REMOVE(&MsgDir, m, entries);
		if(m->read_by != NULL)
			textline_free(m->read_by);
		free(m);
	}
}

void
check_msgdir(void)
{
	struct msg_dir_entry *msg;
	char *oops = NULL;

	TAILQ_FOREACH(msg, &MsgDir, entries) {
		struct msg_dir_entry *next, *prev;
		next = TAILQ_NEXT(msg, entries);
		if (next != NULL) {
			if (TAILQ_PREV(next, MsgList, entries) != msg)
				oops = "msg->next->last != msg";
		} else {
			if(msg != TAILQ_LAST(&MsgDir, MsgList))
				oops = "msg != MsgDirTail";
		}

		prev = TAILQ_PREV(msg, MsgList, entries);
		if(prev != NULL) {
			if(TAILQ_NEXT(prev, entries) != msg)
				oops = "msg->last->next != msg";
		}

		if(oops != NULL) {
			char buf[256];
	
			log_f("msgd", "TERMINAL:", oops);
			snprintf(buf, sizeof(buf), "msg #%ld [0x%p]",
				msg->number, msg);
			log_f("msgd", "TERMINAL:", buf);
			if(next != NULL) {
				snprintf(buf, sizeof(buf),
					"next: 0x%p, next->last: 0x%p",
					next, TAILQ_PREV(next, MsgList, entries));
				log_f("msgd", "TERMINAL:", buf);
			}
			if(prev != NULL) {
				snprintf(buf, sizeof(buf),
					"last: 0x%p, last->next: 0x%p",
					prev, TAILQ_NEXT(prev, entries));
				log_f("msgd", "TERMINAL:", buf);
			}
			log_f("msgd", "ABORTING", "bye");
			exit(1);
		}
	}

}

int
compress_messages(struct active_processes *ap)
{
	struct msg_dir_entry *msg;
	int number = 1;

	fwddir_open();

	bbsd_msg("Begin Compress");
	log_f("msgd", "AGING:", "Start");

	TAILQ_FOREACH(msg, &MsgDir, entries) {
		fwddir_rename(msg->number, number);
		msg_body_rename(msg->number, number);
		number++;
	}
	return OK;
}

void
type_stats(void)
{
	struct msg_dir_entry *msg;
	int type, class;
	int cnt[3][4];
	int pers = 0, bull = 0, nts = 0;
	int immune = 0, active = 0, held = 0, killed = 0;
	int i, j;

	for(i=0; i<3; i++)
		for(j=0; j<4; j++)
			cnt[i][j] = 0;

	TAILQ_FOREACH(msg, &MsgDir, entries) {
		switch(msg->flags & MsgTypeMask) {
		case MsgBulletin:
			type = 1;
			bull++;
			break;
		case MsgNTS:
			type = 2;
			nts++;
			break;
		default:
			pers++;
			type = 0;
		}

		if(IsMsgImmune(msg)) {
			cnt[type][2]++;
			immune++;
		} else {
			if(IsMsgKilled(msg)) {
				cnt[type][3]++;
				killed++;			
			} else {
				if(IsMsgHeld(msg)) {
					cnt[type][1]++;
					held++;
				} else {
					cnt[type][0]++;
					active++;
				}
			}
		}
	}
		
	snprintf(output, sizeof(output),
		"\nType\ttotal\tactive\theld\timmune\tkilled\n"
		" P\t%d\t%d\t%d\t%d\t%d\n"
		" B\t%d\t%d\t%d\t%d\t%d\n"
		" T\t%d\t%d\t%d\t%d\t%d\n"
		"\t%d\t%d\t%d\t%d\t%d\n",
		 pers, cnt[0][0], cnt[0][1], cnt[0][2], cnt[0][3],
		 bull, cnt[1][0], cnt[1][1], cnt[1][2], cnt[1][3],
		nts, cnt[2][0], cnt[2][1], cnt[2][2], cnt[2][3],
		pers+bull+nts, active, held, immune, killed);
}

void
age_messages(void)
{
	struct msg_dir_entry *msg, *msg_temp;
	time_t now = Time(NULL);
	int type;
	char log_buf[256];

	fwddir_open();

	bbsd_msg("Begin Aging");
	log_f("msgd", "AGING:", "Start");

	msg = TAILQ_FIRST(&MsgDir);
	while (msg != NULL) {
		struct msg_dir_entry *next, *prev;
		next = TAILQ_NEXT(msg, entries);
		prev = TAILQ_PREV(msg, MsgList, entries);
		snprintf(log_buf, sizeof(log_buf),
			"%05ld [0x%p last: 0x%p next: 0x%p]", 
			msg->number, msg, prev, next);

		switch(msg->flags & MsgTypeMask) {
		case MsgBulletin:
			type = BULL;
			break;
		case MsgNTS:
			type = NTS;
			break;
		default:
			type = PERS;
		}

		if(IsMsgImmune(msg) || fwddir_check(msg->number)) {
			log_f("msgd", "IMMUNE:", log_buf);
			msg = TAILQ_NEXT(msg, entries);
			continue;
		}

		if(IsMsgKilled(msg)) {
			if((now - msg->kdate) > Msgd_Age_Killed[type]) {
				log_f("msgd", "DELETE:", log_buf);
				msg_body_kill(msg->number);
				msg = unlink_msg_list(msg);
				if(msg == NULL) {
					msg = TAILQ_FIRST(&MsgDir);
					continue;
				}
			} else
				log_f("msgd", "killed:", log_buf);
				
		} else
			if(IsMsgActive(msg)) {
				time_t delta = now - msg->cdate;
				time_t live =
					msg->time2live ? msg->time2live : Msgd_Age_Active[type];

				if(delta > live) {
					log_f("msgd", "KILL:", log_buf);
					SetMsgKilled(msg);
					build_list_text(msg);
				} else
					if(delta > (live - Msgd_Age_Old)) {
						log_f("msgd", "old:", log_buf);
						if(!IsMsgOld(msg)) {
							SetMsgOld(msg);
							build_list_text(msg);
						}
					} else
						log_f("msgd", "active:", log_buf);

			} else
				log_f("msgd", "not_active:", log_buf);
		msg = TAILQ_NEXT(msg, entries);
	}
	bbsd_msg("Validate");
	check_msgdir();
	bbsd_msg("");
	log_f("msgd", "AGING:", "End");
}

int
visible_by(struct active_processes *ap, struct msg_dir_entry *msg)
{
	if(ap->grp) {
		struct message_list *ml = ap->grp->list;
		while(ml) {
			if(ml->msg == msg)
				break;
			NEXT(ml);
		}
		if(ml == NULL)
			return ERROR;
	}

	if(ap->list_mode == SysopMode)
		return OK;

	switch(ap->list_mode) {
	case MineMode:
		if(!strcmp(ap->call, msg->to.name.str))
			return OK;
		break;
	default:
		if(msg->cdate <= ap->list_mode)
			return ERROR;
	case NormalMode:
		if(msg->flags & (MsgBulletin|MsgNTS))
			return OK;

		if(!strcmp(ap->call, msg->to.name.str))
			return OK;
		if(!strcmp(ap->call, msg->from.name.str))
			return OK;
	}

	return ERROR;
}

int
read_by_rcpt(struct msg_dir_entry *msg)
{
	struct text_line *by = msg->read_by;

	while(by) {
		if(!strcmp(by->s, msg->to.name.str))
			return TRUE;
		NEXT(by);
	}
	return FALSE;
}

int
read_by_me(struct msg_dir_entry *msg, char *call)
{
	struct text_line *by = msg->read_by;

	while(by) {
		if(!strcmp(by->s, call))
			return TRUE;
		NEXT(by);
	}
	return FALSE;
}

void
build_list_text(struct msg_dir_entry *msg)
{
	msg->flags &= ~MsgReadByMe;

	if(msg->passwd[0])
		snprintf(msg->list_text, sizeof(msg->list_text),
			"%ld %ld %ld !%s $%s %s@%s %s %"PRTMd" %ld %s\n",
			msg->number, msg->size, msg->flags, msg->passwd, msg->bid,
			msg->to.name.str, msg->to.at.str, msg->from.name.str,
			msg->cdate, msg->read_cnt, msg->sub);
	else
		snprintf(msg->list_text, sizeof(msg->list_text),
			"%ld %ld %ld $%s %s@%s %s %"PRTMd" %ld %s\n",
			msg->number, msg->size, msg->flags, msg->bid,
			msg->to.name.str, msg->to.at.str, msg->from.name.str,
			msg->cdate, msg->read_cnt, msg->sub);

	msg->update_t = TIME;
}

char *
build_display(struct active_processes *ap, struct msg_dir_entry *msg)
{
	static char buf[256];
	char stat[5];
	char datebuf[20];
	char password[20];
	struct tm *dt = localtime(&(msg->cdate));

	msg->flags &= ~MsgReadByMe;
	if(read_by_me(msg, ap->call))
		msg->flags |= MsgReadByMe;

	switch(ap->disp_mode) {
	case dispBINARY:
		snprintf(buf, sizeof(buf), "BINARY %ld\n", msg->number);
		break;
	case dispVERBOSE:
		if(IsMsgSecure(msg)) stat[0] = 'S';
		else if(IsMsgPersonal(msg)) stat[0] = 'P';
		else if(IsMsgBulletin(msg)) stat[0] = 'B';
		else if(IsMsgNTS(msg)) stat[0] = 'T';
		else stat[0] = '?';

		if(IsMsgPending(msg)) stat[1] = 'P';
		else if(IsMsgNoFwd(msg)) stat[1] = 'N';
		else stat[1] = ' ';

		if(IsMsgCheckedOut(msg)) stat[2] = 'C';
		else if(IsMsgImmune(msg)) stat[2] = 'I';
		else if(IsMsgOld(msg)) stat[2] = 'O';
		else if(IsMsgHeld(msg)) stat[2] = 'H';
		else if(IsMsgKilled(msg)) stat[2] = 'K';
		else stat[2] = ' ';

		switch(msg->flags & (MsgRead | MsgReadByMe)) {
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
		snprintf(buf, sizeof(buf),
			"%5ld %s %5ld %6s@%-6s %-6s%3ld %9s %-.28s\n",
			msg->number, stat, msg->size, msg->to.name.str,
			msg->to.at.str, msg->from.name.str, msg->read_cnt,
			datebuf, msg->sub);
		break;

	case dispNORMAL:
		password[0] = 0;
		if(msg->passwd[0])
			snprintf(password, sizeof(password), " !%s",
				msg->passwd);

		if(ap->list_mode == SysopMode)
			snprintf(buf, sizeof(buf),
				"%ld %ld %ld%s $%s %s@%s %s %"PRTMd" %ld %s\n",
				msg->number, msg->size, msg->flags, password, msg->bid,
				msg->to.name.str, msg->to.at.str, msg->from.name.str,
				msg->cdate, msg->read_cnt, msg->sub);
		else
			snprintf(buf, sizeof(buf),
				"%ld %ld %ld%s $ %s@%s %s %"PRTMd" %ld %s\n",
				msg->number, msg->size, msg->flags, password, 
				msg->to.name.str, msg->to.at.str, msg->from.name.str,
				msg->cdate, msg->read_cnt, msg->sub);
		break;
	}

	return buf;
}

void
show_message(struct active_processes *ap, struct msg_dir_entry *msg)
{
	if(ap->disp_mode == dispNORMAL) {
		if(read_by_me(msg, ap->call))
			socket_raw_write(ap->fd, "R");
		socket_raw_write(ap->fd, msg->list_text);

		{
			char buf[256];
			strlcpy(buf, msg->list_text, sizeof(buf));
			log_f("msgd", "L:", buf);
		}
	} else {
		char *buf = build_display(ap, msg);
		socket_raw_write(ap->fd, buf);

		if (buf[0] != '\0')
			buf[strlen(buf)-1] = 0;
		log_f("msgd", "L:", buf);
	}
}

struct msg_dir_entry *
get_message(int number)
{
	struct msg_dir_entry *msg;

	TAILQ_FOREACH(msg, &MsgDir, entries) {
		if(msg->number == number)
			return msg;
	}
	return NULL;
}

void
list_messages(struct active_processes *ap)
{
		if(ap->grp) {
			struct message_list *ml = ap->grp->list;
			while(ml) {
				if(ml->msg->update_t > ap->list_sent)
					if(ml->msg->flags & (MsgActive|MsgHeld))
						show_message(ap, ml->msg);
				NEXT(ml);
			}
		} else {
			struct msg_dir_entry *msg;
			if(ap->list_mode == BbsMode) {
				fwddir_open();
				TAILQ_FOREACH(msg, &MsgDir, entries) {
					if(fwddir_check(msg->number))
						show_message(ap, msg);
				}
				fwddir_close();
				ap->list_mode = SysopMode;
			} else
				TAILQ_FOREACH(msg, &MsgDir, entries) {
					if(msg->update_t > ap->list_sent)
						if(visible_by(ap, msg) == OK)
							show_message(ap, msg);
				}
		}
	ap->list_sent = Time(NULL);
}

void
read_who(struct active_processes *ap, struct msg_dir_entry *msg)
{
	struct text_line *by = msg->read_by;
	char buf[80];

	while(by) {
		snprintf(buf, sizeof(buf), "%s\n", by->s);
		socket_raw_write(ap->fd, buf);
		NEXT(by);
	}
}

int
send_message(struct active_processes *ap)
{
	FILE *fp;
	struct msg_dir_entry *msg, *prev;
	int in_rfc = FALSE;
	int dup = FALSE;
	int nl, next_nl;
	char buf[4096];
	
	msg = append_msg_list();
	prev = TAILQ_PREV(msg, MsgList, entries);
	if(prev != NULL)
		msg->number = prev->number + 1;
	else
		msg->number = 1;

	if((fp = open_message_write(msg->number)) == NULL)
		return ERROR;
	
	for (nl = 1; TRUE; nl = next_nl) {
		switch(socket_read_raw_line(ap->fd, buf, 4096, 10)) {
		case sockOK:
			next_nl = 1;
			break;
		case sockMAXLEN:
			next_nl = 0;
			break;
		case sockERROR:
		case sockTIMEOUT:
			return ERROR;
		}

		log_f("msgd", "r:", buf);

		if(nl && !strcmp(buf, ".\n"))
			break;

		if(nl && !strcmp(buf, "..\n"))
			strcpy(buf, ".\n");

		if(nl && in_rfc) {
			char rfc[256];
			int has_nl;

			strlcpy(rfc, buf, sizeof(rfc));

			switch(rfc822_parse(msg, rfc)) {
			case rBID:
				if(!strcmp(msg->bid, "$\n")) {
					snprintf(msg->bid, sizeof(msg->bid),
						"%ld_%s", msg->number,
						Bbs_Call);
					rfc822_gen(rBID, msg, buf, 80);
					strlcat(buf, "\n", sizeof(buf));
				}
				if(bid_chk(msg->bid))
					dup = TRUE;
				else
					bid_add(msg->bid);
				break;
			case rCREATE:
			case rKILL:
				has_nl = rfc[strlen(rfc)-1] == '\n';
				snprintf(buf, sizeof(buf), "%s%s", rfc, has_nl ? "" : "\n");
				break;
			}
		} else if(nl && !strcmp(buf, "/EX\n"))
			in_rfc = TRUE;

		fputs(buf, fp);
	}

	if(dup) {
		fprintf(fp, "%s MSGD\n", rfc822_xlate(rHELDBY));
		fprintf(fp, "%s Duplicate bid detected inside message daemon\n",
			rfc822_xlate(rHELDWHY));
		fprintf(fp, "%s This is probably a result of a message being\n",
			rfc822_xlate(rHELDWHY));
		fprintf(fp, "%s received via two ports simultaneously\n",
			rfc822_xlate(rHELDWHY));
	}
	fclose(fp);

	if(rfc822_decode_fields(msg) != OK)
		return ERROR;

	if(IsMsgBulletin(msg))
		set_groups(msg);
	if(msg->flags & MsgActive)
		set_forwarding(ap, msg, FALSE);
	build_list_text(msg);

	return msg->number;
}

int
is_number(char *s)
{
	while(*s) {
		if(!isdigit(*s))
			return FALSE;
		s++;
	}
	return TRUE;
}

void
check_route(struct active_processes *ap, char *s)
{
	struct msg_dir_entry *msg;
	char buf[80];

	uppercase(s);
	if(is_number(s))
		if((msg = get_message(atoi(s))) != NULL) {
			set_forwarding(ap, msg, TRUE);
			return;
		}
		
	msg = malloc_struct(msg_dir_entry);
	snprintf(buf, sizeof(buf), "To: %s\n", s);
	rfc822_parse(msg, buf);
	set_forwarding(ap, msg, TRUE);
	free(msg);
}

void
clean_users(void)
{
	struct active_processes *ap = procs;
	while(ap) {
		ap->list_sent = FALSE;
		NEXT(ap);
	}
}

