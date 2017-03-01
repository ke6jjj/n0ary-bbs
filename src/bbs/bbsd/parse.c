#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"
#include "daemon.h"

int operation = FALSE;
extern int shutdown_daemon;

static char *
convert_time(void)
{
	char *result;
	if((result = config_fetch("DEBUG_TIME")) == NULL)
		return "0\n";
	snprintf(output, sizeof(output), "%"PRTMd"\n", str2time_t(result));
	return output;
}

static char *
parse_check(char *s)
{
	char call[80];
	struct Ports *port = NULL;
	struct active_processes *ap = procs;
	int cnt = 0;

	if(*s == 0)
		return Error("Expected a call");

	strncpy(call, get_string(&s), sizeof(call));
	if(*s != 0)
		port = locate_port(get_string(&s));

	uppercase(call);

	while(ap) {
		if(!strcmp(ap->call, call)) {
			if(port != NULL) {
				if(ap->via == port)
					cnt++;
			} else
				cnt++;
		}
		NEXT(ap);
	}

	snprintf(output, sizeof(output), "%d\n", cnt);
	return output;
}

static char *
parse_status(struct active_processes *me, char *s)
{
	char buf[4096];
	char *mode = get_string(&s);
	uppercase(mode);

	if(!strcmp(mode, "PROCS")) {
		struct active_processes *ap = procs;
		struct Ports *port = PortList;

		while(ap) {
			if(ap->verbose == TRUE) {
				long t = time(NULL);
				sprintf(buf, "%d %s %s %d %d %ld %ld %s %s\n",
					ap->proc_num, ap->call, ap->via->name,
					ap->chat_port, ap->pid, t - ap->t0, t - ap->idle,
					ap->chat?"YES":"NO", ap->text ? ap->text:".");
			} else {
				sprintf(buf, "%d %s %s %d %d %ld %ld %s %s\n",
					ap->proc_num, ap->call, ap->via->name,
					ap->chat_port, ap->pid, ap->t0, ap->idle,
					ap->chat?"YES":"NO", ap->text ? ap->text:".");
			}
			if(write(me->fd, buf, strlen(buf)) < 0)
				break;
			NEXT(ap);
		}

		while(port) {
			if(port->lock)
				sprintf(buf, "%s LOCK %s\n", port->name, port->reason);
			else
				sprintf(buf, "%s UNLOCK\n", port->name);
			if(write(me->fd, buf, strlen(buf)) < 0)
				break;
			NEXT(port);
		}

		return ".\n";
	}

	if(!strcmp(mode, "LOCK")) {
		struct Ports *port = PortList;
		while(port) {
			if(port->lock)
				sprintf(buf, "%s LOCK %s\n", port->name, port->reason);
			else
				sprintf(buf, "%s UNLOCK\n", port->name);
			if(write(me->fd, buf, strlen(buf)) < 0)
				break;
			NEXT(port);
		}

		return ".\n";
	}

	return Error("illegal option to STATUS command");
}

static char *
help_disp(void)
{
	output[0] = 0;
	strcat(output, "LOGIN call via  ........ request to run\n");
	strcat(output, "PORT number  ........... set your chat port\n");
	strcat(output, "PID number  ............ proc id of client\n");
	strcat(output, "CHECK call [via] ....... is user connected\n");
	strcat(output, "MSG string  ............ display my message\n");
	strcat(output, "PING  .................. reset bbsd timeout\n");
	strcat(output, "ALERT string  .......... display a message in console\n");
	strcat(output, "CHAT ON|OFF ............ set/clear CHAT request\n");
	strcat(output, "SET variable value  .... set a bbs wide variable\n");
	strcat(output, "SHOW variable  ......... get a bbs wide variable\n");
	strcat(output, "SHOWORIG variable ...... get original bbs variable\n");
	strcat(output, "\n");
	strcat(output, "LOCK via reason  ....... lock port\n");
	strcat(output, "UNLOCK via  ............ unlock port\n");
	strcat(output, "\n");
	strcat(output, "NOTIFY ON|OFF .......... put in/out of notify mode\n");
	strcat(output, "STATUS PROCS  .......... show all procs and locks\n");
	strcat(output, "STATUS LOCK via  ....... show if port is locked\n");
	strcat(output, "STATUS LOCK  ........... show lock status of all ports\n");
	strcat(output, "TIME ................... return time value\n");
	strcat(output, "VERBOSE ON|OFF ......... talking to user, rather than pgm\n");
	strcat(output, ".\n");
	return output;
}

char *
set_parameters(char *s)
{
	char opt[40];
	strcpy(opt, get_string(&s));
	uppercase(opt);

	if(!strcmp(opt, "LOG")) {
		if(*s && *s != 0) {
			strcpy(opt, get_string(&s));
			uppercase(opt);
			if(!strcmp(opt, "ON"))
				Logging = logON;
			if(!strcmp(opt, "CLR"))
				Logging = logONnCLR;
			if(!strcmp(opt, "OFF"))
				Logging = logOFF;
		}
		switch(Logging) {
		case logOFF:
			return Ok("logging is off");
		case logON:
			return Ok("logging is on");
		case logONnCLR:
			return Ok("logging is on with clearing enabled");
		}
		return Ok("unknown logging status");
	}
	return Error("Invalid option");
}

char *
parse(struct active_processes *ap, char *s)
{
	char *result, buf[80], cmd[80];

	struct BbsdCommands *bc = BbsdCmds;

	ap->idle = time(NULL);
	strcpy(buf, get_string(&s));

	if(buf[0] == '?')
		return help_disp();

	strcpy(cmd, buf);
	uppercase(cmd);

	if(!strcmp(cmd, "EXIT"))
		return NULL;

	if(!strcmp(cmd, "SHUTDOWN")) {
		shutdown_daemon = TRUE;
		return NULL;
	}
	if(!strcmp(buf, "DEBUG"))
		return set_parameters(s);

	while(bc->token != 0) {
		if(!stricmp(bc->key, buf)) {
			char *p, buf[256];
			struct Ports *via;

			switch(bc->token) {
			case bCHAT:
				if(!stricmp(s, "ON"))
					ap->chat = TRUE;
				else
					ap->chat = FALSE;
				sprintf(buf, "CHAT %d %d %s",
					ap->proc_num, ap->chat_port, ap->chat?"ON":"OFF");
				textline_append(&Notify, buf);
				return JustOk;
				
			case bCHECK:
				return parse_check(s);

			case bLOCK:
				via = locate_port(get_string(&s));
				via->lock = TRUE;
				if(via->reason)
					free(via->reason);
				via->reason = (char*)malloc(strlen(s)+1);
				strcpy(via->reason, s);
				return JustOk;

			case bLOGIN:
				if(ap->call[0] != 0) {
					sprintf(buf, "LOGOUT %d", ap->proc_num);
					textline_append(&Notify, buf);
				}

				strcpy(ap->call, get_string(&s));
				uppercase(ap->call);
				if((ap->via = locate_port(get_string(&s))) == NULL)
					return Error("invalid port");
						
				sprintf(buf, "LOGIN %d %s %s %ld",
					ap->proc_num, ap->call, ap->via->name, ap->t0);
				textline_append(&Notify, buf);

				daemon_check_in(ap);

				if(ap->via->lock == TRUE) {
					sprintf(output, "NO, %s\n", ap->via->reason);
					return output;
				}
				return JustOk;
				
			case bMSG:
				if(ap->text)
					free(ap->text);

				ap->text = (char*)malloc(strlen(s)+1);
				strcpy(ap->text, s);
				sprintf(buf, "MSG %d %s", ap->proc_num, ap->text);
				textline_append(&Notify, buf);
				return JustOk;

			case bPING:
				return NoResponse;

			case bALERT:
				if(ap->text)
					free(ap->text);

				ap->text = (char*)malloc(strlen(s)+1);
				strcpy(ap->text, s);
				sprintf(buf, "MSG %d %s", ap->proc_num, ap->text);
				textline_append(&Notify, buf);
				return JustOk;

			case bNOTIFY:
				if(!stricmp(s, "ON"))
					ap->notify = TRUE;
				else
					ap->notify = FALSE;
				return JustOk;

			case bVERBOSE:
				if(!stricmp(s, "ON"))
					ap->verbose = TRUE;
				else
					ap->verbose = FALSE;
				return JustOk;

			case bPORT:
				ap->chat_port = atoi(s);
				sprintf(buf, "PORT %d %d",
					ap->proc_num, ap->chat_port);
				daemon_up(ap);
				textline_append(&Notify, buf);
				return JustOk;

			case bPID:
				ap->pid = atoi(s);
				sprintf(buf, "PID %d %d",
					ap->proc_num, ap->pid);
				textline_append(&Notify, buf);
				return JustOk;

			case bSET:
				p = get_string(&s);
				if(config_override(p, s) == ERROR)
					return Error("not a valid variable");
				return JustOk;
				
			case bSHOW:
				if((result = config_fetch(get_string(&s))) == NULL)
					return Error("Variable not found");
				sprintf(output, "%s\n", result);
				return output;

			case bSHOWLIST:
				if(config_fetch_list(get_string(&s)) == 0)
					return Error("Variable not found");
				return output;

			case bTIME:
				return convert_time();

			case bSHOWORIG:
				if((result = config_fetch_orig(get_string(&s))) == NULL)
					return Error("Variable not found");
				sprintf(output, "%s\n", result);
				return output;

			case bSTATUS:
				return parse_status(ap, s);

			case bUNLOCK:
				via = locate_port(get_string(&s));
				via->lock = FALSE;
				if(via->reason)
					free(via->reason);
				return JustOk;
			}
		}
		bc++;
	}
	return Error("illegal command");
}

