#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "tools.h"
#include "config.h"
#include "bbslib.h"
#include "tokens.h"
#include "history.h"
#include "function.h"

struct history_log {
	struct history_log *next, *last;
	int number;
	char cmd[1024];
} *Hist, *hist;

int cmd_number = 1;

static int
	hist_depth = 0;

void
history_init(int depth)
{
	int i;
	Hist = hist = malloc_multi_struct(depth, history_log);

	for(i=1; i<depth; i++) {
		hist[i].number = 0;
		hist[i].last = &hist[i-1];
		hist[i-1].next = &hist[i];
	}

	hist[0].number = 0;
	hist[0].last = &hist[depth-1];
	hist[depth-1].next = &hist[0];
	hist_depth = depth;
}

int
history(void)
{
	struct TOKEN *t = TokenList;
	struct history_log *h = hist;
	int number = hist->number;
	int cnt = 15;

	NEXT(t);
	if(t->token == NUMBER)
		if(t->value <= 40)
			cnt = t->value;

	while(cnt--)
		PREV(h);

	do {
		NEXT(h);
		if(h->number)
			PRINTF("%3d %s\n", h->number, h->cmd);
	} while(h->number != number);
	return OK;
}

void
history_add(char *cmd)
{
	NEXT(hist);
	hist->number = cmd_number++;
	strcpy(hist->cmd, cmd);
}

char *
history_cmd(int num)
{
	int i;
	struct history_log *h = hist;
	for(i=0; i<hist_depth; i++) {
		if(h->number == num)
			return h->cmd;
		PREV(h);
	}
	return NULL;
}

char *
history_last_cmd(void)
{
	return hist->cmd;
}
