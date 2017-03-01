#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "server.h"
#include "user.h"
#include "message.h"
#include "vars.h"

char InServer = 0;

void init_server(char *server, int msg_number, char *text);

int
initiate(void)
{
	char *server;
	struct TOKEN *t = TokenList;
	NEXT(t);

	switch(t->token) {
	case WP:
		server = "WP";
		break;
	case CALLBK:
		server = "CALLBK";
		break;

	case FORWARD:
		NEXT(t);
		return OK;

	case EVENT:
	case FILESYS:
		system_msg_string(71, t->lexem);
		return OK;
	default:
		return bad_cmd(-1, t->location);
	}

	NEXT(t);

	while(t->token != END) {
#ifdef NOMSGD
		struct message_directory_entry m;
		if(t->token != NUMBER)
			return bad_cmd(-1, t->location);

		m.number = t->value;
		if(get_msg_directory_entry(&m) == OK) {
			init_server(server, m.number, m.sub);
		}
#else
		struct msg_dir_entry *m;
		if(t->token != NUMBER)
			return bad_cmd(-1, t->location);

		if((m = msg_locate(t->value)) != NULL)
			init_server(server, m->number, m->sub);
#endif
		NEXT(t);
	}
	return OK;
}

void
init_server(char *server, int msg_number, char *text)
{
	char buf[80];
	char cmd[80];
	sprintf(cmd, "%s/bin/b_bbs", Bbs_Dir);
	sprintf(buf, "%d", msg_number);

	switch(*server) {
	case 'C':
		if(!fork()) {
			execl(cmd, "b_bbs", "-t3", buf, text, 0);
			exit(0);
		}
		break;	
	case 'W':
		if(!fork()) {
			case_string(text, AllUpperCase);
			execl(cmd, "b_bbs", "-t4", buf, text, 0);
			exit(0);
		}
		break;	

	}
}
