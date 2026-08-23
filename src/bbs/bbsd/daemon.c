#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"
#include "daemon.h"

#define dmnDOWN		0
#define dmnSTART	1
#define dmnATTACH	2
#define dmnONLINE	3

struct Daemon {
	SLIST_ENTRY(Daemon) entries;
	char *name;
	char *cmd;
	int state;
	int proc_num;
	int respawn;
	time_t timeout;
};

SLIST_HEAD(daemon_list, Daemon);

static struct daemon_list DaemonList = SLIST_HEAD_INITIALIZER(DaemonList);

static struct Daemon *initialize_daemon(struct text_line *tl);

static void
daemon_display(struct Daemon *dl)
{
	char *p = "HUH?"; 
	switch(dl->state) {
	case dmnDOWN: p = "Down"; break;
	case dmnSTART: p = "Start"; break;
	case dmnATTACH: p = "Attach"; break;
	case dmnONLINE: p = "Online"; break;
	}

	printf("\t%s\t%s%4d  RESPAWN:%sTO:%4"PRTMd"  %s\n",
		dl->name, p,
		dl->proc_num, dl->respawn ? "YES ":"NO  ",
		dl->timeout, dl->cmd);
}

static struct Daemon *
initialize_daemon(struct text_line *tl)
{
	struct Daemon *dmn;
	FILE *fp;
	char *value, *s, *name, *cmd;

	s = tl->s;
	value = get_string(&s);
	if (s == NULL) {
		log_warning("DAEMON %s no command found\n", value);
		goto NoName;
	}

	if ((name = strdup(value)) == NULL) {
		log_error("Unable to allocate daemon name.");
		goto NameAlloc;
	}
	if ((cmd = strdup(s)) == NULL) {
		log_error("Unable to allocate daemon cmd.");
		goto CmdAlloc;
	}

	value = get_string(&s);
	/* check for existance of program */
	if((fp = fopen(value, "r")) == NULL) {
		log_debug("DAEMON %s program not found %s", name, value);
		goto NoExe;
	}
	fclose(fp);

	if ((dmn = malloc_struct(Daemon)) == NULL) {
		log_error("Unable to allocate daemon.");
		goto DmnAlloc;
	}

	dmn->name = name;
	dmn->cmd = cmd;
	if(!strcmp(dmn->name, "IGNORE"))
		dmn->respawn = FALSE;
	else
		dmn->respawn = TRUE;
	dmn->state = dmnDOWN;

	return dmn;

DmnAlloc:
NoExe:
	free(cmd);
CmdAlloc:
	free(name);
NameAlloc:
NoName:
	return NULL;
}

int
build_daemon_list(void)
{
	struct text_line *dtl = NULL, *tl;
	int cnt = 0;
	
	if((cnt = config_fetch_multi("DAEMON", &dtl)) == 0)
		return cnt;

	tl = dtl;
	while(tl) {
		struct Daemon *dmn = initialize_daemon(tl);
		if (dmn != NULL) {
			SLIST_INSERT_HEAD(&DaemonList, dmn, entries);
			cnt++;
		}

		NEXT(tl);
	}
	textline_free(dtl);

	if(dbug_level & 0x10) {
		struct Daemon *dl;
		printf("Daemon List:\n");
		SLIST_FOREACH(dl, &DaemonList, entries) {
			daemon_display(dl);
		}
		printf(".\n");
		fflush(stdout);
	}
	return cnt;
}

static void
start_daemon(char *cmd)
{
	char cmdline[256];
	sprintf(cmdline, "%s", cmd);
	if(Bbsd_Port != BBSD_PORT)
		sprintf(cmdline, "%s -p%d", cmdline, Bbsd_Port);
	if(strcmp(Bbs_Host, BBS_HOST))
		sprintf(cmdline, "%s -h%s", cmdline, Bbs_Host);
	strcat(cmdline, " 2>/dev/null &");
	if(dbug_level & 0x10)
		printf("starting: %s\n", cmdline);
	system(cmdline);
}

int
daemons_start(void)
{
	struct Daemon *dl;

	if(dbug_level & 0x20) {
		printf("."); fflush(stdout);
	}

	SLIST_FOREACH(dl, &DaemonList, entries) {
		if(dl->state == dmnDOWN) {
			start_daemon(dl->cmd);
			dl->state = dmnSTART;
			dl->timeout = bbs_time(NULL) + 30;
			return TRUE;
		}
	}
	return FALSE;
}

int
daemons_check(void)
{
	struct Daemon *dl;
	time_t now = bbs_time(NULL);

	SLIST_FOREACH(dl, &DaemonList, entries) {
		struct active_process *ap;
		if(!strcmp(dl->name, "IGNORE")) {
			continue;
		}

		LIST_FOREACH(ap, &procs, entries) {
			if(!strcmp(dl->name, ap->call))
				break;
		}
		if(ap == NULL) {
				/* not found, must not be up */
			if(dl->timeout < now) {
				start_daemon(dl->cmd);
				dl->timeout = now + 30;
				dl->state = dmnDOWN;
			}
		}
	}
	return OK;
}

int
daemon_up(struct active_process *ap)
{
	int down_cnt = 0;
	struct Daemon *dl;

	SLIST_FOREACH(dl, &DaemonList, entries) {
		if(!strcmp(ap->call, dl->name)) {
			dl->state = dmnONLINE;
			dl->proc_num = ap->proc_num;

			if(dbug_level & 0x10) {
				printf("Daemon Up:");
				daemon_display(dl);
				fflush(stdout);
			}
		}

		if((dl->state != dmnONLINE) && dl->respawn)
			down_cnt++;
	}

	if(down_cnt == 0)
		lock_clear_all();
	return OK;
}

int
daemon_check_in(struct active_process *ap)
{
	struct Daemon *dl;

	SLIST_FOREACH(dl, &DaemonList, entries) {
		if(!strcmp(ap->call, dl->name)) {
			dl->state = dmnATTACH;
			dl->proc_num = ap->proc_num;

			if(dbug_level & 0x10) {
				printf("Daemon Checkin:");
				daemon_display(dl);
				fflush(stdout);
			}
		}
	}
	return OK;
}

int
daemon_check_out(struct active_process *ap)
{
	struct Daemon *dl;

	SLIST_FOREACH(dl, &DaemonList, entries) {
		if(ap->proc_num == dl->proc_num) {
			long now = bbs_time(NULL);
			dl->state = dmnDOWN;
			dl->proc_num = 0;
			dl->timeout = now + 30;

			if(dbug_level & 0x10) {
				printf("Daemon Checkout:");
				daemon_display(dl);
			}

			if(dl->respawn)
				start_daemon(dl->cmd);
			break;
		}
	}
	return OK;
}

void
daemon_respawn(char *name, int state)
{
	struct Daemon *dl;

	SLIST_FOREACH(dl, &DaemonList, entries) {
		if(!strcmp(name, dl->name)) {
			dl->respawn = state;
			break;
		}
	}
}
