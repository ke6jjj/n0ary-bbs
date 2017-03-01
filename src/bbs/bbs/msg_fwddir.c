#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "system.h"
#include "msg_fwddir.h"
#include "function.h"

struct MessageQueue *queue = NULL;

static int pending_cnt;
static char *alias;

void
fwddir_messages_for(char *str)
{
	struct MessageQueue **q = &queue;

	while(*q)
		q = &((*q)->next);

	*q = malloc_struct(MessageQueue);	
	(*q)->number = atoi(str);
	strcpy((*q)->alias, alias);
	pending_cnt++;
}

void
fwddir_queue_free(void)
{
	while(queue) {
		struct MessageQueue *q = queue;
		NEXT(queue);
		free(q);
	}
}

int
fwddir_queue_messages(struct System_alias *name, char *order)
{
	pending_cnt = 0;
	fwddir_queue_free();

	if(*order != '*') {
		while(*order) {
			struct System_alias *cname = name;
			if(!isalpha(*order)) {
				order++;
				continue;
			}

			while(cname) {
				char cmd[80];
				alias = cname->alias;
				sprintf(cmd, "%s %s %c", 
					msgd_xlate(mPENDING), cname->alias, *order);
				msgd_fetch_multi(cmd, fwddir_messages_for);
				NEXT(cname);
			}
			order++;
		}
	} else
		while(name) {
			char cmd[80];
			alias = name->alias;
			sprintf(cmd, "%s %s", msgd_xlate(mPENDING), name->alias);
			msgd_fetch_multi(cmd, fwddir_messages_for);
			NEXT(name);
		}

	return pending_cnt;
}

