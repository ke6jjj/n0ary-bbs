#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "user.h"
#include "message.h"
#include "system.h"
#include "msg_fwddir.h"
#include "vars.h"
#include "version.h"
#include "msg_read.h"
#include "filesys.h"
#include "bbscommon.h"
#include "help.h"

static int
	header,
	touch,
	why,
	who,
	killmsg,
	mine;

static int who_has_read(int num);
static int read_message_number(int num);
static int read_mine(void);
static int read_messages_who(struct TOKEN *t);
static int read_messages_why(struct TOKEN *t);
static int display_routing(struct msg_dir_entry *m, struct text_line **tl,
	int mode);
static int build_send_cmd(struct msg_dir_entry *m, char *cmd, size_t sz);
static int format_routing_timestamp(char *buf, size_t size, struct tm *dt);

int
msg_read_t(struct TOKEN *t)
{
	int read_cnt = 0;

	header = FALSE;
	who = FALSE;
	why = FALSE;
	mine = FALSE;
	killmsg = FALSE;
	touch = FALSE;

	while(t->token != END) {
		switch(t->token) {
		case HEADER:
			header = TRUE;
			break;
		case MINE:
			if(read_cnt)
				return bad_cmd(131, t->location);
			mine = TRUE;
			break;

		case KILL:
			killmsg = TRUE;
			break;

		case MOTD:
		case COMMA:
			break;
		case NOPROMPT:
			no_prompt = TRUE;
			break;
		case WHO:
			who = TRUE;
			break;
		case WHY:
			why = TRUE;
			break;
		case TOUCH:
			touch = TRUE;
			break;
		case NUMBER:
			if(mine)
				return bad_cmd(131, t->location);
			read_cnt++;
			break;

		case WORD:
		case STRING:
			return file_read_t(t);

		default:
			return bad_cmd(130, t->location);
		}
		NEXT(t);
	}

	if(mine)
		read_mine();
	else {
		if(!read_cnt)
			if(append_active_message())
				return error(77);

		if(who)
			read_messages_who(TokenList);
		else
			if(why)
				read_messages_why(TokenList);
			else
				read_messages(TokenList);
	}

	return OK;
}

static int
read_mine(void)
{
	struct list_criteria lc;
	struct msg_dir_entry *m;
	int first_time = TRUE;

	bzero(&lc, sizeof(lc));
	lc.must_include_mask = MsgMine;
	lc.exclude_mask = MsgReadByMe;
	lc.include_mask = MsgActive | MsgHeld;

	if(build_partial_list(&lc) == 0) {
		system_msg(145);
		return OK;
	}

	TAILQ_FOREACH(m, &MsgDirList, entries) {
		if(m->visible == FALSE) {
			continue;
		}

		if(!first_time)
			if(conditional_more())
				return OK;
		first_time = FALSE;

		if(who)
			who_has_read(m->number);
		else
			read_message_number(m->number);

		if(port_type(Via) == tTNC)
			sleep(4);
	}
	return OK;
}

void
read_messages(struct TOKEN *t)
{
#ifdef THRASH_MSGD
	build_full_message_list();
#endif

	while(t->token != END) {
		if(t->token == NUMBER) {
			if(Base && (t->value < 1000))
				t->value += (Base * 1000);

			read_message_number(t->value);

			if(t->next->token != END && !touch)
				if(conditional_more())
					return;
		}
		NEXT(t);

		if(port_type(Via) == tTNC)
			sleep(2);
	}
}

static int
read_messages_why(struct TOKEN *t)
{
	while(t->token != END) {
		if(t->token == NUMBER) {
			int num = t->value;
			if(Base && (t->value < 1000))
				num += (Base * 1000);
			if(msg_hold_disp_reason(num) == ERROR)
				return ERROR;
			active_message = num;
		}
		NEXT(t);
	}
	return OK;
}

static int
read_messages_who(struct TOKEN *t)
{
	while(t->token != END) {
		if(t->token == NUMBER) {
			int num = t->value;
			if(Base && (t->value < 1000))
				num += (Base * 1000);
			who_has_read(num);
			active_message = num;
		}
		NEXT(t);
	}
	return OK;
}

static void
disp_who(char *s)
{
	PRINTF("%s ", s);
}

static int
who_has_read(int num)
{
	struct msg_dir_entry *m;

	if((m = msg_locate(num)) == NULL)
		return error_number(132, num);

	PRINTF("Msg: %5d To: %s@%s Fr: %s Sj: %s\nRead By:",
		m->number, m->to.name.str, m->to.at.str, m->from.name.str, m->sub);

	if(m->read_cnt > 0) {
		char cmd[80];
		snprintf(cmd, sizeof(cmd), "%s %d", msgd_xlate(mWHO), num);
		msgd_fetch_multi(cmd, disp_who);
		PRINT("\n");
	} else
		PRINT(" Nobody\n");

	return OK;
}

/* Messages should look like this:
 * -----------------------
 * R:92....
 * R:92...
 * R:92....
 *				<- blank required
 * Message body is here
 * /EX
 * From:
 * To:
 * Subject:
 * X-Bid:
 */

static int
read_message_number(int num)
{
	struct msg_dir_entry *m = msg_locate(num);
	struct text_line *tl;

	if(m == NULL) {
		system_msg_number(132, num);
		return ERROR;
	}

	active_message = num;

	if(!ImSysop && (m->flags & MsgHeld)) {
		PRINTF("Message number %d has been held for sysop review.\n", num);
		PRINTF("To see the reason the message has been held type\n");
		PRINTF("  READ WHY %d\n", num);
		return OK;
	}

	if(IsMsgSecure(m)) {
		if(!port_secure(Via))
			return error(81);
		if(IsMsgPassword(m)) {
			char passwd[80];
			system_msg(82);
			if (GETS(passwd, 79) == NULL)
				return ERROR;
			case_string(passwd, AllUpperCase);
			if(strcmp(m->passwd, passwd))
				if(!ImSysop || strcmp(passwd, "OVERRIDE"))
					return error(83);
		}
	}

	if(msg_ReadBody(m) == ERROR)
		exit(1);
	if(msg_ReadRfc(m) == ERROR)
		exit(1);
	tl = m->body;

	init_more();
	if(ImBBS) {
		PRINTF("%s\n", m->sub);
		display_routing(m, &tl, TRUE);
		while(tl) {
			if(!strncmp(tl->s, "***", 3))
				PRINT(">");
			PRINTF("%s\n", tl->s);
			NEXT(tl);
		}
		PRINT("\x1a\n");

	} else {
		char *field = NULL;
		char datebuf[40];
		struct text_line *rfc = m->header;
		struct tm *dt = localtime(&(m->cdate));
		strftime(datebuf, 40, "%e %h %Y %H:%M", dt);

		PRINTF("=============================================================\n");
		if(more()) return OK;
		PRINTF("      Date: %s\n", datebuf); if(more()) return OK;
		PRINTF(" Message #: %d\n", m->number); if(more()) return OK;
		if(m->flags & MsgBid) {
			PRINTF("       BID: <%s>\n", m->bid); if(more()) return OK;
		}
		field = rfc822_find(rFROM, rfc);
		if(field != NULL) {
			PRINTF("      %s\n", field);
			if(more()) return OK;
		}
		field = rfc822_find(rTO, rfc);
		if(field != NULL) {
			PRINTF("        %s\n", field);
			if(more()) return OK;
		}
		PRINTF("   Subject: %s\n", m->sub); if(more()) return OK;
		if(display_routing(m, &tl, header))
			return OK;
		PRINTF("-------------------------------------------------------------\n");
		if(more())
			return OK;

		while(tl) {
			PRINTF("%s\n", tl->s);
			if(more())
				return OK;
			NEXT(tl);
		}
		PRINTF("\n");
		more();
	}

	if(killmsg && ImSysop)
		msgd_cmd_num(msgd_xlate(mKILL), m->number);
	return OK;
}

static int
display_routing(struct msg_dir_entry *m, struct text_line **tl, int mode)
{
	int cnt = 0;
	char datebuf[40];
	time_t t = Time(NULL);
	struct tm *dt = gmtime(&t);
	format_routing_timestamp(datebuf, sizeof(datebuf), dt);

	if(ImBBS)
		PRINTF("R:%s %d@%s.%s %s\n",
			datebuf, m->number, Bbs_Call, Bbs_Hloc, Bbs_Header_Comment);
	else {
		PRINTF("\n"); if(more()) return ERROR;
	}


	while(*tl) {
		if(strncmp((*tl)->s, "R:", 2))
			break;
		if(mode) {
			PRINTF("%s\n", (*tl)->s);
			if(!ImBBS) if(more()) return ERROR;
		} else {
			char rh[256];
			char *p, *q;
			strncpy(rh, (*tl)->s, sizeof(rh));

			p = (char *)index(rh, '@');
			if(p != NULL) {
				while(!isalnum(*p)) p++;
				q = p;
				while(isalnum(*p)) p++;
				*p = 0;

				if(cnt == 8)
					PRINTF("!%s ...", q);
				else
					if(cnt == 0) {
						PRINTF("%s", q); 
						if(more())
							return ERROR;
					} else
						if(cnt < 8)
							PRINTF("!%s", q);
			}
		}
		(*tl) = (*tl)->next;
		cnt++;
	}
	if(!ImBBS) {
		PRINTF("\n"); if(more()) return ERROR;
	}
	return OK;
}

static int
build_send_cmd(struct msg_dir_entry *m, char *cmd, size_t sz)
{
	switch(m->flags & MsgTypeMask) {
	case MsgPersonal:
		strncpy(cmd, "SP", sz);
		break;
	case MsgBulletin:
		strncpy(cmd, "SB", sz);
		break;
	case MsgNTS:
		strncpy(cmd, "ST", sz);
		break;

	default:
		msgd_cmd_num(msgd_xlate(mFORWARD), m->number);
		return ERROR;
	}

	if(m->to.at.str[0])
		snprintf(cmd, sz, "%s %s @ %s", cmd, m->to.name.str, m->to.at.str);
	else
		snprintf(cmd, sz, "%s %s", cmd, m->to.name.str);

	if(ISupportHloc) {
		if(m->to.address[0])
			snprintf(cmd, sz, "%s.%s", cmd, m->to.address);
	} else {
		char buf[256];
		snprintf(buf, sizeof(buf), "Message #%ld being forwarded to %s", m->number, usercall);
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__, 
				   "!ISupportHloc", buf, "");
		msg_hold_by_bbs(m->number, "BBS doesn't support HLOC? .. don't activate");
		exit(5);
	}

	if(ISupportBID) {
		if(m->bid[0])
			snprintf(cmd, sz, "%s < %s $%s\n", cmd, m->from.name.str, m->bid);
		else
			snprintf(cmd, sz, "%s < %s\n", cmd, m->from.name.str);
	} else {
		char buf[256];
		snprintf(buf, sizeof(buf), "Message #%ld being forwarded to %s", m->number, usercall);
		bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__, 
				   "!ISupportBID", buf, m->bid);
		msg_hold_by_bbs(m->number, "BBS doesn't support BID? .. don't activate");
		exit(5);
	}

	return OK;
}

int
msg_title_write(int num, FILE *fp)
{
	struct msg_dir_entry *m;
	char datebuf[40];
	struct tm *dt;

	if((m = msg_locate(num)) == NULL) {
		system_msg_number(132, num);
		return ERROR;
	}

	dt = localtime(&(m->cdate));
	strftime(datebuf, sizeof(datebuf), "[%Y-%m-%dT%H:%M]", dt);

	fprintf(fp, "Msg: %ld %s", m->number, datebuf);

	if(m->bid[0])
		fprintf(fp, " BID: %s", m->bid);
	else
		fprintf(fp, " BID: [none]");

	fprintf(fp, "\n");
	return OK;
}

int
msg_forward(
	int (*cmd_upcall)(char *buf),
	int (*body_upcall)(char *buf),
	int (*term_upcall)(void)) 
{
	struct msg_dir_entry *m;
	char logbuf[256];
	int result = OK;

	while(queue) {
		if((m = msg_locate(queue->number)) == NULL) {

				/* if message wasn't found then let's just skip it
				 * for this forwarding run. could be that it is being
				 * received right now.
				 */
			NEXT(queue);
			continue;
		}


		if((m->flags & MsgActive) && !(m->flags & MsgHeld)) {
			char datebuf[40];
			struct text_line *tl;
			struct tm *dt = gmtime(&(m->cdate));
			char cmd[80], buf[4096];

			if(msg_ReadBodyBy(m, usercall) == ERROR) {
				exit(1);
			}
			if(msg_ReadRfc(m) == ERROR)
				exit(1);

			rfc822_parse(m, rfc822_find(rTO, m->header));

			if(build_send_cmd(m, cmd, sizeof(cmd))) {
				NEXT(queue);
				continue;
			}
			cmd[strlen(cmd)-1] = 0;

			{
				char outbuf[256];
				snprintf(outbuf, sizeof(outbuf), "%ld %s", m->number, cmd);
				bbsd_msg(outbuf);
			}
			if(m->body == NULL) {
				logd("***No message body?");
				exit(1);
			}
			result = cmd_upcall(cmd);

			snprintf(logbuf, sizeof(logbuf), "\t%ld\t%s", m->number, cmd);
			logd(logbuf);

			if(result == ERROR) {
				logd("***Could not issue command");
				break;
			}

			if(result == TRUE) {
				int body_err = FALSE;
				if(debug_level & DBG_MSGFWD)
					PRINTF("%05ld  SEND  %s@%s",
						m->number, m->to.name.str, m->to.at.str);

				if(body_upcall(m->sub) != OK) {
					logd("***Could not issue message subject");
					break;
				}

				format_routing_timestamp(datebuf, sizeof(datebuf), dt);
				if(m->bid[0])
					snprintf(buf, sizeof(buf), "R:%s @:%s.%s [%s] #:%ld $:%s",
						datebuf, Bbs_Call, Bbs_Hloc,
						Bbs_Header_Comment, m->number, m->bid);
				else
					snprintf(buf, sizeof(buf), "R:%s @:%s.%s [%s] #:%ld",
						datebuf, Bbs_Call, Bbs_Hloc,
						Bbs_Header_Comment, m->number);

				if(body_upcall(buf) != OK) {
					logd("***Could not issue our route entry");
					break;
				}

				tl = m->body;
				while(tl) {
					if((body_err = body_upcall(tl->s)) != OK) {
						logd("***Could not issue message body");
						break;
					}
					NEXT(tl);
				}

				if(body_err)
					break;

				if(term_upcall() != OK) {
					logd("***Could not issue message termination");
					break;
				}
			} else
				if(debug_level & DBG_MSGFWD)
					PRINTF("%05ld REJECT %s@%s",
						m->number, m->to.name.str, m->to.at.str);

			result = OK;
			msg_forwarded(m->number, queue->alias);
			if(msg_pending_cnt(m->number) == 0)
				if(m->flags & (MsgNTS | MsgPersonal)) {
					if(debug_level & DBG_MSGFWD)
						PRINTF("  KILLED");
					msgd_cmd_num(msgd_xlate(mKILL), m->number);
				}

			if(debug_level & DBG_MSGFWD) PRINTF("\n");

		} else {
			snprintf(logbuf, sizeof(logbuf), "\t%ld\t %s@%s [%s]", m->number,
				m->to.name.str, m->to.at.str,
				(m->flags & MsgHeld)? "HELD":"KILLED");
			logd(logbuf);
		}

		msg_CatchUp();
		NEXT(queue);
	}

#if 0
	logd("");
#endif
	return result;
}

int
msg_revfwd_cmd(char *buf)
{
	char result[256];

	PRINTF("%s\n", buf);
	snprintf(result, sizeof(result), "%s: %s", usercall, buf);
	bbsd_msg(result);

	if (GETS(result, 255) == NULL)
		return FALSE;
	case_string(result, AllUpperCase);
	if(result[0] == 'O')
		return TRUE;

	do {
		if (GETS(result, 255) == NULL)
			return FALSE;
		case_string(result, AllUpperCase);
	} while(result[0] != 'F' && result[1] != '>');
	return FALSE;
}

int
msg_revfwd_body(char *buf)
{
	if(!strncmp(buf, "***", 3))
		PRINT(">");
	PRINTF("%s\n", buf);
	return OK;
}

int
msg_revfwd_term(void)
{
	char result[80];

	PRINT("\x1a\n");
	do {
		if (GETS(result, 79) == NULL)
			return ERROR;
		case_string(result, AllUpperCase);
	} while(result[0] != 'F' && result[1] != '>');
	return OK;
}

int
msg_revfwd_dumb_term(void)
{
	char result[80];

	PRINT("\x1a\n");
	do {
		if (GETS(result, 79) == NULL)
			return ERROR;
		case_string(result, AllUpperCase);
	} while(result[0] != 'F' && result[1] != '>');
	return OK;
}

int
msg_revfwd(void)
{
	struct System *sys;

	if(!ImBBS)
		return OK;

	logd("msg_revfwd(Begin reverse forwarding)");
	system_open();
	sys = SystemList;
	while(sys) {
		if(!strcmp(sys->alias->alias, usercall))
			break;
		NEXT(sys);
	}

	if(sys) {
		char prefix[80];
		snprintf(prefix, sizeof(prefix), "<%s: ", sys->alias->alias);
		bbsd_prefix_msg(prefix);
		bbsd_msg(" ");
		fwddir_queue_messages(sys->alias, sys->order);
		msg_forward(msg_revfwd_cmd, msg_revfwd_body, msg_revfwd_term);
	}
	PRINTF("*** Done\n");
	exit_bbs();
	return OK;
}

static int
format_routing_timestamp(char *buf, size_t size, struct tm *dt)
{
	return strftime(buf, size, "%y%m%d/%H%Mz", dt);
}
