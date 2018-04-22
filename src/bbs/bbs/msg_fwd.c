#include <stdio.h>
#include <string.h>
#ifndef SUNOS
#include <regex.h>
#endif
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "tokens.h"
#include "function.h"
#include "user.h"
#include "message.h"
#include "server.h"
#include "system.h"
#include "msg_fwddir.h"
#include "version.h"
#include "msg_fwd.h"
#include "msg_read.h"

extern int
	debug_level;

static int
	tnc_fd;

static void
	forward_by_tnc(struct System *bbs),
	forward_by_tcp(struct System *bbs),
	forward_by_phone(struct System *bbs);

extern void
    msg_fwd_by_smtp(struct System *sys);

void
init_forwarding(char *port, char *call)
{
	char buf[80];
	struct System *sys;

	sprintf(buf, "FWD on %s", port);
	logd_open(buf);

	system_open();
	sys = SystemList;

	if(call != NULL)
		uppercase(call);

	while(sys) {
		int msg_cnt;

		if(sys->now == FALSE) {
			NEXT(sys);
			continue;
		}

		if(call)
			if(strcmp(sys->alias->alias, call)) {
				NEXT(sys);
				continue;
			}

		if(strcmp(port, sys->port)) {
			NEXT(sys);
			continue;
		}

		if(bbsd_login(sys->alias->alias, port) != OK) {
			bug_report(Bbs_Call, BBS_VERSION, __FILE__, __LINE__,
				"init_forwarding", sys->alias->alias,
				"Error trying to re-login to bbsd");
			exit(1);
		}

		if(bbsd_check(sys->alias->alias, NULL) != 1) {
			NEXT(sys);
			continue;
		}

		msg_cnt = fwddir_queue_messages(sys->alias, sys->order);
		{
			char prefix[80];
			sprintf(prefix, ">%s: ", sys->alias->alias);
			bbsd_prefix_msg(prefix);
			bbsd_msg(" ");
		}

		sprintf(buf, "%s Pending %d messages", sys->alias->alias, msg_cnt);
		logd(buf);

		if(msg_cnt || (sys->options & optPOLL)) {
			strcpy(usercall, sys->alias->alias);
			user_open();

			switch(port_type(port)) {
			case tTNC:
				forward_by_tnc(sys);
				break;

			case tPHONE:
				forward_by_phone(sys);
				break;

			case tTCP:
				forward_by_tcp(sys);
				break;

			case tSMTP:
				msg_fwd_by_smtp(sys);
				break;
			}

			userd_close();
		} else
			if(debug_level & DBG_MSGFWD)
				PRINTF("No pending activity for %s\n", sys->alias->alias);

		ISupportHloc = FALSE;
		ISupportBID = FALSE;
		ImBBS = FALSE;
		fwddir_queue_free();
		NEXT(sys);
	}

	logd_close();
	system_close();
}

int
tnc_getline(int fd, char *buf, int cnt, int timeout)
{
	switch(socket_read_line(fd, buf, cnt, timeout)) {
	case sockOK:
	case sockMAXLEN:
		return OK;
	case sockTIMEOUT:
		logd("***Timeout reading from remote bbs");
		return ERROR;
	case sockERROR:
		logd("***Error reading from tnc");
		return ERROR;
	}
	return ERROR;
}

int
tcp_getline(int fd, char *buf, int len, int to)
{
	char *p = buf;
	fd_set ready;
	struct timeval timeout;
	int cnt = 0;

	FD_ZERO(&ready);
	FD_SET(fd, &ready);

	timeout.tv_sec = to;
	timeout.tv_usec = 0;

			/* wait upto 30 seconds for the first connect response */

	if(debug_level & DBG_MSGFWD)
		PRINTF("\nR: ");

	while(TRUE) {
		if(select(fd+1, (void*)&ready, 0, 0, &timeout) == 0) {
			if(cnt != 0)
				break;
			logd("***Timeout reading from remote bbs");
			return ERROR;
		}

		if(read(fd, p, 1) == 0) {
			logd("***Error reading from tcp");
			return ERROR;
		}
if(debug_level & DBG_MSGFWD) {
	if(isprint(*p))
		PRINTF("%c", *p);
	else
		PRINTF("[%02x]", (unsigned char)*p);
}
		if(*p == '\r')
			continue;
		if(*p == '\n')
			break;
		p++;
		cnt++;

		if(cnt == len)
			return ERROR;

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
	}
	*p = 0;
	return OK;
}

int
msg_fwd_dumb_cmd(char *buf)
{
	char result[80];

	fd_putln(tnc_fd, buf);

	if(tnc_getline(tnc_fd, result, 80, 300))
		return ERROR;
	return TRUE;
}

int
msg_fwd_cmd(char *buf)
{
	char result[80];

	fd_putln(tnc_fd, buf);

	if(tnc_getline(tnc_fd, result, 80, 300))
		return ERROR;

	case_string(result, AllUpperCase);
	if(result[0] == 'O')
		return TRUE;

	while(TRUE) {
		if(tnc_getline(tnc_fd, result, 80, 300))
			return ERROR;

		if(result[strlen(result)-1] == '>')
			break;
	}
	return FALSE;
}

int
msg_fwd_body(char *buf)
{
	if(!strncmp(buf, "***", 3))
		socket_raw_write(tnc_fd, ">");
	fd_putln(tnc_fd, buf);
	return OK;
}

int
msg_tcp_fwd_body(char *buf)
{
		/* the telnet connection may have problems with line length
		 * this is a patch until I can think of a way around the problem.
		 * simply add a newline if the user hasn't been typing 
		 * carriage returns.
		 */

	if(strlen(buf) > 128) {
		char str[128];
		strncpy(str, buf, 127);
		str[127] = 0;
		fd_putln(tnc_fd, str);
		return msg_tcp_fwd_body(&buf[127]);
	}
	fd_putln(tnc_fd, buf);
	return OK;
}

int
msg_fwd_dumb_term(void)
{
	char result[80];

	fd_putln(tnc_fd, "\n\032");
	while(TRUE) {
		if(tnc_getline(tnc_fd, result, 80, 600))
			return ERROR;

		if(result[strlen(result)-1] == '>')
			break;
	}
	return OK;
}

int
msg_fwd_term_ex(void)
{
	char result[80];

	fd_putln(tnc_fd, "\n/EX");
	
	while(TRUE) {
		if(tnc_getline(tnc_fd, result, 80, 600))
			return ERROR;

		if(result[strlen(result)-1] == '>')
			break;
	}
	return OK;
}

int
msg_fwd_term(void)
{
	char result[80];

	fd_putln(tnc_fd, "\x1a");
	
	while(TRUE) {
		if(tnc_getline(tnc_fd, result, 80, 600))
			return ERROR;

		if(result[strlen(result)-1] == '>')
			break;
	}
	return OK;
}

static void
forward_by_tcp(struct System *sys)
{
	char buf[256];
	int prompt = FALSE;
	int result = OK;
	char *p, host[80];
	int port = 23;
	int timeout = FALSE;
	int sid_found = FALSE;
	struct System_chat *chat = sys->chat;

	p = sys->connect;
	strcpy(host, get_string(&p));
	if(*p)
		port = get_number(&p);

	sprintf(buf, "Connecting to %s @ port %d", host, port);
	bbsd_msg(buf);

	if((tnc_fd = socket_open(host, port)) == ERROR) {
		sprintf(buf, "***Problem connecting to %s", host);
		logd(buf);
		return;
	}

	/* Save the other party's remote address so that we can log it */
	get_remote_addr(tnc_fd, &RemoteAddr);

/* put alarm call here to check for timeout */

#if 0
socket_watcher(tnc_fd);
#endif
	while(chat) {
		int tval;

		switch(chat->dir) {
		case chatRECV:
			while(!timeout) {
				if(tcp_getline(tnc_fd, buf, 256, chat->to)) {
					close(tnc_fd);
					return;
				}
#if 1
				if((re_comp(chat->txt)) != NULL) {
					struct text_line *tl = NULL;
					textline_append(&tl, 
				  "The RECV field  presented in your Systems file doesn't");
					textline_append(&tl, 
                  "appear to be a valid regular expression.");
					textline_append(&tl, chat->txt);
					problem_report_textline(Bbs_Call, BBS_VERSION, 
											usercall, tl);
					textline_free(tl);
					return;
				}
				if(re_exec(buf) == 1)
					break;
#else
				if(strstr(buf, chat->txt))
					break;
#endif

				if((unsigned char)buf[0] == 0xFF && (unsigned char)buf[1] == 0xFD) {
					buf[1] = 0xFC;
					if(debug_level & DBG_MSGFWD)
						PRINT("Sending: [ff][fc][?]\n");
					socket_raw_write(tnc_fd, buf);
				}
					
			}
			break;

		case chatDELAY:
			tval = atoi(chat->txt);
			sprintf(buf, "Delaying %s seconds", chat->txt);
			bbsd_msg(buf);
			sleep(tval);
			break;
			
		case chatSEND:
			fd_putln(tnc_fd, chat->txt);

			sprintf(buf, "Sending %s", chat->txt);
			bbsd_msg(buf);
			break;

		case chatDONE:
			break;
		}
		NEXT(chat);
	}

	sprintf(buf, "Connected to %s", sys->alias->alias);
	bbsd_msg(buf);

	while(!prompt) {

		if(tnc_getline(tnc_fd, buf, 256, 120)) {
			close(tnc_fd);
			return;
		}

		if(buf[strlen(buf)-1] == '>')
			prompt = TRUE;

		if(buf[0] == '[') {
			if(parse_bbssid(buf, sys->sid) == ERROR)
				return;
			sid_found = TRUE;
		}
	}

	if(sid_found == FALSE) {
		logd("No SID received");
#if 0
		problem_report(Bbs_Call, BBS_VERSION, usercall, "No SID received");
#endif
		return;
	}

	if(sys->options & optDUMBTNC)
		result = msg_forward(msg_fwd_dumb_cmd, msg_tcp_fwd_body, msg_fwd_dumb_term);
	else {
		fd_putln(tnc_fd, build_sid());
		while(TRUE) {
			if(tnc_getline(tnc_fd, buf, 256, 30)) {
				close(tnc_fd);
				return;
			}
			if(buf[strlen(buf)-1] == '>')
				break;
		}
		bbsd_msg("Start forwarding");
		result = msg_forward(msg_fwd_cmd, msg_tcp_fwd_body, msg_fwd_term_ex);
	}
#if 0
socket_watcher(ERROR);
#endif

	if(result == OK)
		if(sys->options & optREVFWD) {
			extern void run_reverse_fwd(int fd);
			char prefix[80];
			sprintf(prefix, "<%s: ", sys->alias->alias);
			bbsd_prefix_msg(prefix);
			bbsd_msg(" ");

			sprintf(prefix, "%s\tBegin reverse forwarding",
				sys->alias->alias);
			logd(prefix);
			run_reverse_fwd(tnc_fd);
		}
#if 0
socket_watcher(ERROR);
#endif

	close(tnc_fd);
}

static void
forward_by_tnc(struct System *sys)
{
	char buf[256];
	char last_sent[256];
	int port = tnc_port(sys->port);
	char *host = tnc_host(sys->port);
	int prompt = FALSE;
	int sid_found = FALSE;
	int result = OK;
	struct System_chat *chat = sys->chat;

	strcpy(last_sent, "Nothing yet");

	sprintf(buf, "PAUSE");
	bbsd_msg(buf);
	sleep(5);
	sprintf(buf, "Connecting to %s", sys->connect);
	bbsd_msg(buf);

	if((tnc_fd = tnc_connect(host, port, sys->connect, Bbs_FwdCall)) == ERROR) {
		sprintf(buf, "***Problem connecting to %s", sys->connect);
		logd(buf);
		return;
	}

	snprintf(buf, sizeof(buf), "ax25:%s", sys->connect);
	parse_remote_addr(buf, &RemoteAddr);

	tnc_set_ax25(&sys->ax25);

	while(chat) {

		switch(chat->dir) {
		case chatRECV:
			if(tnc_getline(tnc_fd, buf, 256, chat->to)) {
				sprintf(buf,
					"***Last sent: %s\n***Timeout (%ld secs) waiting for: %s",
					last_sent, chat->to, chat->txt);
				logd(buf);
				close(tnc_fd);
				return;
			}

#if 1
			if((re_comp(chat->txt)) != NULL) {
				struct text_line *tl = NULL;
				textline_append(&tl, 
				  "The RECV field  presented in your Systems file doesn't");
				textline_append(&tl, 
                  "appear to be a valid regular expression.");
				textline_append(&tl, chat->txt);
				problem_report_textline(Bbs_Call, BBS_VERSION, 
										usercall, tl);
				textline_free(tl);
				return;
			}
			if(re_exec(buf) != 1) {
				close(tnc_fd);
				return;
			}
#else
			if(!strstr(buf, chat->txt)) {
				close(tnc_fd);
				return;
			}
#endif
			break;

		case chatSEND:
			fd_putln(tnc_fd, chat->txt);

			sprintf(buf, "Sending %s", chat->txt);
			bbsd_msg(buf);
			strcpy(last_sent, chat->txt);
			break;

		case chatDONE:
			break;
		}
		NEXT(chat);
	}

	sprintf(buf, "Connected to %s", sys->alias->alias);
	bbsd_msg(buf);

	while(!prompt) {

		if(tnc_getline(tnc_fd, buf, 256, 120)) {
			char lbuf[256];
			sprintf(lbuf, "***Error on tnc_getline");
			logd(lbuf);
			close(tnc_fd);
			return;
		}

		if(buf[strlen(buf)-1] == '>')
			prompt = TRUE;

		if(buf[0] == '[') {
			if(parse_bbssid(buf, sys->sid) == ERROR)
				return;
			sid_found = TRUE;
		}
	}

	if(sid_found == FALSE) {
		problem_report(Bbs_Call, BBS_VERSION, usercall, "No SID received");
		return;
	}

	if(sys->options & optDUMBTNC)
		result = msg_forward(msg_fwd_dumb_cmd, msg_fwd_body, msg_fwd_dumb_term);
	else {
		fd_putln(tnc_fd, build_sid());
		while(TRUE) {
			if(tnc_getline(tnc_fd, buf, 256, 60)) {
				char lbuf[256];
				sprintf(lbuf, "***Error on tnc_getline");
				logd(lbuf);
				close(tnc_fd);
				return;
			}
			if(buf[strlen(buf)-1] == '>')
				break;
		}
		result = msg_forward(msg_fwd_cmd, msg_fwd_body, msg_fwd_term);
	}

	if(result == OK)
		if(sys->options & optREVFWD) {
			extern void run_reverse_fwd(int fd);
			char prefix[80];
			sprintf(prefix, "<%s: ", sys->alias->alias);
			bbsd_prefix_msg(prefix);
			bbsd_msg(" ");

			sprintf(prefix, "%s Begin reverse forwarding",
				sys->alias->alias);
			logd(prefix);
			run_reverse_fwd(tnc_fd);
		}

	close(tnc_fd);
}

/*ARGSUSED*/
static void
forward_by_phone(struct System *sys)
{
}

void
display_routes(char *s)
{
	PRINTF("%s\n", s);
}

int
msg_chk_route_t(struct TOKEN *t)
{
	char cmd[80];

	switch(t->token) {
	case NUMBER:
		sprintf(cmd, "%s %d", msgd_xlate(mROUTE), t->value);
		break;

	case STRING:
	case WORD:
		sprintf(cmd, "%s %s", msgd_xlate(mROUTE), &CmdLine[t->location]);
		break;

	default:
		return bad_cmd(130, t->location);
	}

	msgd_fetch_multi(cmd, display_routes);
	return OK;
}

