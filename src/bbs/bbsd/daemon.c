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

struct Daemons {
	struct Daemons *next;
	char *name;
	char *cmd;
	int state;
	int proc_num;
	int respawn;
	time_t timeout;
};

static struct Daemons
	*DmnList = NULL,
	*DmnListEnd = NULL;

static void
daemon_display(struct Daemons *dl)
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

int
build_daemon_list(void)
{
	struct text_line *dtl = NULL, *tl;
	int cnt = 0;
	
	if((cnt = config_fetch_multi("DAEMON", &dtl)) == 0)
		return cnt;

	tl = dtl;
	while(tl) {
		char *value, *s = tl->s;
		struct Daemons *dmn = malloc_struct(Daemons);
		int bad = FALSE;

		value = get_string(&s);
		if(*s) {
			FILE *fp;
			dmn->name = (char*)malloc(strlen(value)+1);
			strcpy(dmn->name, value);
			dmn->cmd = (char*)malloc(strlen(s)+1);
			strcpy(dmn->cmd, s);

			if(!strcmp(dmn->name, "IGNORE"))
				dmn->respawn = FALSE;
			else
				dmn->respawn = TRUE;
			dmn->state = dmnDOWN;

			value = get_string(&s);
					/* check for existance of program */
			if((fp = fopen(value, "r")) == NULL) {
				if(VERBOSE)
					printf("DAEMON %s program not found %s\n", dmn->name, value);
				free(dmn->name);
				free(dmn->cmd);
				bad = TRUE;
			} else {
				fclose(fp);
				if(DmnListEnd == NULL)
					DmnList = dmn;
				else
					DmnListEnd->next = dmn;
				DmnListEnd = dmn;
				cnt++;
			}
		} else {
			if(VERBOSE)
				printf("DAEMON %s no command found\n", value);
			bad = TRUE;
		}

		if(bad)
			free(dmn);

		NEXT(tl);
	}
	textline_free(dtl);

	if(dbug_level & 0x10) {
		struct Daemons *dl = DmnList;
		printf("Daemon List:\n");
		while(dl) {
			daemon_display(dl);
			NEXT(dl);
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
	strcat(cmdline, " &");
	if(dbug_level & 0x10)
		printf("starting: %s\n", cmdline);
	system(cmdline);
}

int
daemons_start(void)
{
	struct Daemons *dl = DmnList;

if(dbug_level & 0x20) {
printf("."); fflush(stdout);
}

	while(dl) {
		if(dl->state == dmnDOWN) {
			start_daemon(dl->cmd);
			dl->state = dmnSTART;
			dl->timeout = bbs_time(NULL) + 30;
			return TRUE;
		}
		NEXT(dl);
	}
	return FALSE;
}
int
daemons_check(void)
{
	struct Daemons *dl = DmnList;
	time_t now = bbs_time(NULL);

	while(dl) {
		struct active_process *ap;
		if(!strcmp(dl->name, "IGNORE")) {
			NEXT(dl);
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

		NEXT(dl);
	}
	return OK;
}

int
daemon_up(struct active_process *ap)
{
	int down_cnt = 0;
	struct Daemons *dl = DmnList;

	while(dl) {
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

		NEXT(dl);
	}

	if(down_cnt == 0)
		lock_clear_all();
	return OK;
}

int
daemon_check_in(struct active_process *ap)
{
	struct Daemons *dl = DmnList;

	while(dl) {
		if(!strcmp(ap->call, dl->name)) {
			dl->state = dmnATTACH;
			dl->proc_num = ap->proc_num;

			if(dbug_level & 0x10) {
				printf("Daemon Checkin:");
				daemon_display(dl);
				fflush(stdout);
			}
		}
		NEXT(dl);
	}
	return OK;
}

int
daemon_check_out(struct active_process *ap)
{
	struct Daemons *dl = DmnList;

	while(dl) {
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
		NEXT(dl);
	}
	return OK;
}

void
daemon_respawn(char *name, int state)
{
	struct Daemons *dl = DmnList;

	while(dl) {
		if(!strcmp(name, dl->name)) {
			dl->respawn = state;
			break;
		}
		NEXT(dl);
	}
}
