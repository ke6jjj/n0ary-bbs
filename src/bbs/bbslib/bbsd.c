#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <malloc.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

struct BbsdCommands BbsdCmds[] = {
	{ bALERT,	"ALERT" },
	{ bCHAT,	"CHAT" },
	{ bCHECK,	"CHECK" },
	{ bLOCK,	"LOCK" },
	{ bLOGIN,	"LOGIN" },
	{ bMSG,		"MSG" },
	{ bNOTIFY,	"NOTIFY" },
	{ bPID,		"PID" },
	{ bPING,	"PING" },
	{ bPORT,	"PORT" },
	{ bSET,		"SET" },
	{ bSHOW,	"SHOW" },
	{ bSHOWLIST,"SHOWLIST" },
	{ bSHOWORIG,"SHOWORIG" },
	{ bSTATUS,	"STATUS" },
	{ bTIME,	"TIME" },
	{ bUNLOCK,	"UNLOCK" },
	{ bVERBOSE,	"VERBOSE" },
	{ 0,		NULL }
};

static int
	bbsd_ver = ERROR,
	bbsd_sock = ERROR;

static char *
	msg_prefix = NULL;

static int proc_num = 0;

int
bbsd_socket(void)
{
	return bbsd_sock;
}

int
bbsd_pid(void)
{
	return bbsd_cmd_num(bbsd_xlate(bPID), getpid());
}

int
bbsd_version(void)
{
	return bbsd_ver;
}

int
bbsd_ping(void)
{
	char buf[1024];

	if(bbsd_sock == ERROR)
		return error_log("bbsd_cmd: bbsd socket not opened");

	sprintf(buf, "%s\n", bbsd_xlate(bPING));
	if(socket_raw_write(bbsd_sock, buf) != OK)
		return error_log("bbsd_cmd: error writing to socket");
	return OK;
}

int
bbsd_login(char *name, char *via)
{
	char buf[256], *result;

	sprintf(buf, "%s %s %s", bbsd_xlate(bLOGIN), name, via);
	if((result = bbsd_fetch(buf)) == NULL)
		return ERROR;

	if(!strncmp(result, "NO,", 3)) {
		error_log("Login rejected because: %s", result);
		return TRUE;
	}
	return OK;
}

int
bbsd_open(char *host, int port, char *name, char *via)
{
	char buf[256], *result;

	if((bbsd_sock = socket_open(host, port)) == ERROR)
		return error_log("bbsd_open(%s, %d): socket open failed", host, port);

		/* read version number of bbsd, ignore for now */

	if((result = bbsd_read()) == NULL)
		return error_log("bbsd_open(%s, %d): error reading from socket",
			host, port);

	if(strncmp(&result[1], "bbsd", 4))
		return error_log(
			"bbsd_open(%s, %d): expected bbsd in version string, got %s",
			host, port, result);
	bbsd_ver = get_daemon_version(result);

	if((result = bbsd_read()) == NULL)
		return error_log("bbsd_open(%s, %d): error reading from socket",
			host, port);

	
	if(*result != '#')
		return error_log("bbsd_open(%s, %d): expected proc_num from bbsd",
			host, port);

	proc_num = atoi(&result[2]);

#if 0
	sprintf(buf, "%s %s %s", bbsd_xlate(bLOGIN), name, via);
	if((result = bbsd_fetch(buf)) == NULL)
		return ERROR;

	if(!strncmp(result, "NO,", 3)) {
		error_log("Login rejected because: %s", result);
		return TRUE;
	}
	return OK;
#else
	return bbsd_login(name, via);
#endif
}

void
bbsd_close(void)
{
	close(bbsd_sock);
	bbsd_sock = ERROR;
}

char *
bbsd_read(void)
{
	static char buf[1024];

	switch(socket_read_line(bbsd_sock, buf, 1023, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		buf[1023] = 0;
		break;
	case sockTIMEOUT:
	case sockERROR:
		error_log("bbsd_read: error reading from socket");
		return NULL;
	}
	return buf;
}

char *
bbsd_xlate(int token)
{
	struct BbsdCommands *c = BbsdCmds;

	while(c->token && c->token != token)
		c++;
	return c->key;
}

int
bbsd_cmd(char *cmd)
{
	char *c, buf[1024];

	if(bbsd_sock == ERROR)
		return error_log("bbsd_cmd: bbsd socket not opened");

	sprintf(buf, "%s\n", cmd);
	if(socket_raw_write(bbsd_sock, buf) != OK)
		return error_log("bbsd_cmd: error writing to socket");

	if((c = bbsd_read()) == NULL)
		return ERROR;

	if(c[0] == 'O' && c[1] == 'K')
		return OK;
	return ERROR;
}

int
bbsd_cmd_num(char *cmd, int num)
{
	char str[1024];

	sprintf(str, "%s %d", cmd, num);
	return bbsd_cmd(str);
}

int
bbsd_cmd_string(char *cmd, char *s)
{
	char str[1024];

	sprintf(str, "%s %s", cmd, s);
	return bbsd_cmd(str);
}

char *
bbsd_fetch(char *cmd)
{
	char *c, buf[1024];

	if(bbsd_sock == ERROR) {
		error_log("bbsd_fetch: bbsd socket not opened");
		return NULL;
	}

	sprintf(buf, "%s\n", cmd);
	if(socket_raw_write(bbsd_sock, buf) != OK) {
		error_log("bbsd_fetch: error writing to socket");
		return NULL;
	}

	c = bbsd_read();
	return c;
}

int
bbsd_fetch_multi(char *cmd, void (*callback)(char *s))
{
	char *c, buf[1024];

	if(bbsd_sock == ERROR)
		return error_log("bbsd_fetch_multi: bbsd socket not opened");

	sprintf(buf, "%s\n", cmd);
	socket_raw_write(bbsd_sock, buf);

	while(TRUE) {
		if((c = bbsd_read()) == NULL) {
			bbsd_close();
			return ERROR;
		}

		if(c[0] == '.' && c[1] == 0)
			break;

		if(!strncmp(c, "NO,", 3))
			return ERROR;
		callback(c);
	}
	return OK;
}

int
bbsd_fetch_textline(char *cmd, struct text_line **tl)
{
	char *c, buf[1024];

	if(bbsd_sock == ERROR)
		return error_log("bbsd_fetch_textline: bbsd socket not opened");

	sprintf(buf, "%s\n", cmd);
	socket_raw_write(bbsd_sock, buf);

	while(TRUE) {
		if((c = bbsd_read()) == NULL) {
			bbsd_close();
			return ERROR;
		}

		if(c[0] == '.' && c[1] == 0)
			break;

		if(!strncmp(c, "NO,", 3))
			return ERROR;
		textline_append(tl, c);
	}
	return OK;
}

int
bbsd_port(int number)
{
	return bbsd_cmd_num(bbsd_xlate(bPORT), number);
}

int
bbsd_lock(char *via, char *reason)
{
	char buf[1024];
	sprintf(buf, "%s %s", via, reason);
	return bbsd_cmd_string(bbsd_xlate(bLOCK), buf);
}

int
bbsd_unlock(char *via)
{
	return bbsd_cmd_string(bbsd_xlate(bUNLOCK), via);
}

int
bbsd_set_variable(char *var, char *val)
{
	char cmd[1024];
	sprintf(cmd, "%s %s", var, val);
	return bbsd_cmd_string(bbsd_xlate(bSET), cmd);
}

char *
bbsd_get_variable(char *var)
{
	char cmd[1024];
	sprintf(cmd, "%s %s", bbsd_xlate(bSHOW), var);
	return bbsd_fetch(cmd);
}

int
bbsd_get_variable_list(char *var, void (*callback)(char *s))
{
	char cmd[1024];
	sprintf(cmd, "%s %s", bbsd_xlate(bSHOWLIST), var);
	return bbsd_fetch_multi(cmd, callback);
}

char *
bbsd_get_orig_variable(char *var)
{
	char cmd[1024];
	sprintf(cmd, "%s %s", bbsd_xlate(bSHOWORIG), var);
	return bbsd_fetch(cmd);
}

int
bbsd_check(char *call, char *via)
{
	char cmd[1024];
	sprintf(cmd, "%s %s", bbsd_xlate(bCHECK), call);
	if(via != NULL)
		sprintf(cmd, "%s %s", cmd, via);

	return atoi(bbsd_fetch(cmd));
}

int
bbsd_chk_lock(char *via)
{
	char *result, cmd[1024];
	sprintf(cmd, "%s LOCK %s", bbsd_xlate(bSTATUS), via);


	result = bbsd_fetch(cmd);
	if(*result == 'L')
		return TRUE;
	return FALSE;
}	

int
bbsd_get_status(struct text_line **tl)
{
	char cmd[1024];
	sprintf(cmd, "%s PROCS", bbsd_xlate(bSTATUS));
	return bbsd_fetch_textline(cmd, tl);
}

long
bbsd_get_time(void)
{
	char *result = bbsd_fetch(bbsd_xlate(bTIME));

	if(result)
		return atol(result);
	return 0;
}

int
bbsd_chat_request(void)
{
	return bbsd_cmd_string(bbsd_xlate(bCHAT), "ON");
}

int
bbsd_chat_cancel(void)
{
	return bbsd_cmd_string(bbsd_xlate(bCHAT), "OFF");
}

int
bbsd_notify_on(void)
{
	if(bbsd_sock == ERROR) {
		error_log("bbsd_notify_on: bbsd socket not opened");
		return ERROR;
	}

	bbsd_cmd_string(bbsd_xlate(bNOTIFY), "ON");
	return bbsd_sock;
}

int
bbsd_notify_off(void)
{
	if(bbsd_sock == ERROR) {
		error_log("bbsd_notify_on: bbsd socket not opened");
		return ERROR;
	}

	bbsd_cmd_string(bbsd_xlate(bNOTIFY), "OFF");
	return OK;
}

int
bbsd_get_configuration(struct ConfigurationList *cl)
{
	char buf[1024];

	while(cl->type != tEND) {
		char *p, result[256];

			/* skip embedded comments */

		if(cl->type == tCOMMENT) {
			cl++;
			continue;
		}

		strcpy(result, bbsd_get_variable(cl->token));

		if(strncmp(result, "NO,", 3)) {
			switch(cl->type) {
			case tTIME:
				p = result;
				*((int*)cl->ptr) = get_number(&p);
				uppercase(p);
				if(*p == 'S') {						/* seconds */
					break;
				}
				if(*p == 'M' && *(p+1) == 'I') {	/* minutes */
					*((int*)cl->ptr) *= tMin;
					break;
				}
				if(*p == 'H') {						/* hours */
					*((int*)cl->ptr) *= tHour;
					break;
				}
				if(*p == 'D') {						/* days */
					*((int*)cl->ptr) *= tDay;
					break;
				}
				if(*p == 'W') {						/* weeks */
					*((int*)cl->ptr) *= tWeek;
					break;
				}
				if(*p == 'M' && *(p+1) == 'O') {	/* months */
					*((int*)cl->ptr) *= tMonth;
					break;
				}
				if(*p == 'Y') {						/* years */
					*((int*)cl->ptr) *= tYear;
					break;
				}
				break;
			case tINT:
				*((int*)cl->ptr) = atoi(result);
				break;
			case tSTRING:
				*((char**)cl->ptr) = malloc(strlen(result)+1);
				strcpy(*((char**)cl->ptr), result);
				break;
			case tDIRECTORY:
			case tFILE:
				if(result[0] == '/')
					strcpy(buf, result);
				else
					sprintf(buf, "%s/%s", bbsd_get_variable("BBS_DIR"), result);
				*((char**)cl->ptr) = malloc(strlen(buf)+1);
				strcpy(*((char**)cl->ptr), buf);
				break;

			default:
				error_log("bbsd_get_configuration: invalid type field");
				return ERROR;
			}
		}
		cl++;
	}
	return OK;
}

char *
bbsd_prefix_msg(char *str)
{
	if(str == NULL)
		return msg_prefix;

	if(msg_prefix != NULL)
		free(msg_prefix);

	if((msg_prefix = (char *)malloc(strlen(str)+1)) != NULL)
		strcpy(msg_prefix, str);
	return msg_prefix;
}

int
bbsd_msg(char *str)
{
	char buf[80], *s;

	if(msg_prefix != NULL) {
		s = (char *)malloc(strlen(msg_prefix)+strlen(str)+1);
		strcpy(s, msg_prefix);
		strcat(s, str);
		strncpy(buf, s, 79);
		free(s);
	} else
		strncpy(buf, str, 79);
	buf[79] = 0;

	return bbsd_cmd_string(bbsd_xlate(bMSG), buf);
}

int
bbsd_alert(char *str)
{
	char buf[80];
	strncpy(buf, str, 79);
	buf[79] = 0;
	return bbsd_cmd_string(bbsd_xlate(bALERT), str);
}
