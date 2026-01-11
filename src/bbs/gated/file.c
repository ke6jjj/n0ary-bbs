#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "date.h"

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "gate.h"

static int iscall(char *s);
static int hash_init(void);
static int write_comment(FILE *fp);

struct gate_entry
	*GateList = NULL,
	*GateHash[128];

int image = DIRTY;

int
hash_key(char *s)
{
	return (int)toupper(*s);
}

static int
iscall(char *s)
{
	int len = strlen(s);
	int alpha = 0;
	int digit = 0;
	int usa = FALSE;

	if(len > 6 || len < 3)
		return FALSE;
	if(*s == 'A' || *s == 'N' || *s == 'K' || *s == 'W') {
		if(len < 4)
			return FALSE;
		usa = TRUE;
	}

	while(*s) {
		if(isalpha(*s))
			alpha++;
		else
			if(isdigit(*s))
				digit++;
			else
				return FALSE;
		s++;
	}

	if(usa && digit != 1)
		return FALSE;

	if(alpha < 2 || digit == 0)
		return FALSE;
	return TRUE;
}

int
add_user(struct gate_entry *g)
{
	struct gate_entry *t = GateList;

	if(t == NULL) {
		t = GateList = g;
		return OK;
	}

	while(TRUE) {
		if(!strcmp(t->call, g->call))
			break;
		if(t->next == NULL) {
			t->next = g;
			return OK;
		}
		NEXT(t);
	}

	while(TRUE) {
		if(!strcmp(t->addr, g->addr))
			return ERROR;
		
		if(t->chain == NULL) {
			t->chain = g;
			return OK;
		}

		t = t->chain;
	}
}

int
add_address(struct gate_entry *g)
{
	int indx = hash_key(g->addr);
	struct gate_entry *t = GateHash[indx];

	if(GateHash[indx] == NULL) {
		GateHash[indx] = g;
		return OK;
	}

	while(t->hash)
		t = t->hash;

	t->hash = g;
	return OK;
}

int
read_file(void)
{
	char s[256];
	int cnt = 0, i;
	FILE *fp = fopen(Gated_File, "r");
	time_t t0;
	int version = 0;

	if(fp == NULL) {
		log_error("Couldn't open %s: %m", Gated_File);
		if(write_file() == ERROR)
			return log_error("Couldn't create %s", Gated_File);
		return read_file(); /* Could be better written -JSC */
	}

	if(dbug_level)
		t0 = time(NULL);

    if(fgets(s, 256, fp) == 0) {
		fclose(fp);
		return log_error("%s is empty", Gated_File);
	}

	if(s[2] == 'v')
		version = atoi(&s[3]);

	for(i=0; i<128; i++)
		GateHash[i] = NULL;
	image = CLEAN;

	while(fgets(s, 256, fp)) {
		char buf[80];
		char *p = s;
		struct gate_entry *g;

		s[strlen(s) - 1] = 0;
		if(*s == '#' || *s == 0)
			continue;

		strcpy(buf, get_string(&p));

		if(iscall(buf) == FALSE)
			continue;

		g = malloc_struct(gate_entry);

		uppercase(buf);
		buf[LenCALL-1] = 0;
		strcpy(g->call, buf);

		if(*p == 0) {
			free(g);
			continue;
		}

		strcpy(buf, get_string(&p));
		buf[LenEMAIL-1] = 0;
		strcpy(g->addr, buf);

		if(*p == 0) {
			free(g);
			continue;
		}

		g->seen = date2time(get_string(&p));
		if(g->seen == 0)
			g->seen = Time(NULL);

		if(version > 0)
			g->warn_sent = date2time(get_string(&p));

		if(add_user(g) == OK) {
			add_address(g);
			cnt++;
			continue;
		}

		free(g);
	}

	if(dbug_level)
		printf("Loaded %d translations in %"PRTMd" seconds\n", cnt, time(NULL) - t0);
	fclose(fp);
	return OK;
}

static int
hash_init(void)
{
	struct gate_entry *g = GateList;
	int i;

	while(g) {
		struct gate_entry *t, *a = g->chain;
		while(a) {
			t = a;
			a = a->chain;
			free(t);
		}
		t = g;
		NEXT(g);
		free(t);
	}
		
	for(i=0; i<128; i++)
		GateHash[i] = NULL;
	GateList = NULL;
	return OK;
}

void
read_new_file(void)
{
	hash_init();
	read_file();
}

static int
write_comment(FILE *fp)
{
    return fputs(
        "# This is a machine created file. If you edit it manually\n"
        "# you need to kill the gated process, edit the file, then\n"
        "# restart the gated daemon.\n"
        "#\n"

        "# This file is automatically updated once an hour if changes\n"
        "# have been made to the runtime memory image.\n"
        "#\n",
        fp
    );
}


int
write_file(void)
{
	FILE *fp;
	struct gate_entry *g = GateList;
	int res;

	if((fp = spool_fopen(Gated_File)) == NULL)
		return log_error("Couldn't create %s for writing", Gated_File);

	if (fprintf(fp, "# v1 %s\n#\n", Gated_File) < 0)
		goto WriteFailed;
	if (write_comment(fp) < 0)
		goto WriteFailed;
	while(g) {
		struct gate_entry *a = g;
		while(a) {
			res = fprintf(fp,
				"%s\t%s\t%s\t%s\n",
				a->call, a->addr, time2date(a->seen),
				time2date(a->warn_sent)
			);
			if (res < 0)
				goto WriteFailed;
			a = a->chain;
		}
		NEXT(g);
	}
		
	if (spool_fclose(fp) < 0)
		goto CloseFailed;

	return OK;

WriteFailed:
	spool_abort(fp);
CloseFailed:
	return ERROR;
}

int
write_if_needed(void)
{
	if(image == DIRTY)
		if(write_file() == OK)
			image = CLEAN;
	return OK;
}

char *
disp_stat()
{
	struct gate_entry *g = GateList;
	int calls = 0;
	int addrs = 0;

	while(g) {
		struct gate_entry *a = g;
		calls++;
		while(a) {
			addrs++;
			a = a->chain;
		}
		NEXT(g);
	}

	sprintf(output, "%d calls, %d addresses, image is %s\n",
		calls, addrs, (image == CLEAN) ? "CLEAN" : "DIRTY");
	return output;
}
