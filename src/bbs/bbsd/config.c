#include <stdio.h>
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"

struct configuration_list {
	struct configuration_list *next;
	struct configuration_list *parent;
	char *token;
	char *value;
	char *newvalue;
} 
	*CList = NULL,
	*CLend = NULL;

static void
append(char *t, char *v)
{
	struct configuration_list *cl = malloc_struct(configuration_list);

	cl->token = (char *)malloc(strlen(t)+1);
	cl->value = (char *)malloc(strlen(v)+1);

	if(CLend == NULL)
		CList = cl;
	else
		CLend->next = cl;

	strcpy(cl->token, t);
	strcpy(cl->value, v);
	CLend = cl;
}

static void
port_define(char *s)
{
	char *c;
	struct Ports **port = &PortList;

	while(*port)
		port = &((*port)->next);
	*port = malloc_struct(Ports);

	c = (*port)->name = copy_string(get_string(&s));

	if(!strcmp(c, "STATUS") || !strcmp(c, "DAEMON"))
		(*port)->lockable = FALSE;
	else
		(*port)->lockable = TRUE;
	(*port)->lock = FALSE;
	(*port)->reason = NULL;
}

int
read_config_file(char *fn)
{
	char buf[256];
	FILE *fp = fopen(fn, "r");

	if(fp == NULL)
		return ERROR;

	while(fgets(buf, 256, fp)) {
		char *p, *token;
		if(!isalpha(buf[0]))
			continue;

		buf[strlen(buf)-1] = 0;

		p = buf;
		token = get_string(&p);
		uppercase(token);
		kill_trailing_spaces(token);
		append(token, p);

		if(!strcmp(token, "PORT"))
			port_define(p);
	}
	fclose(fp);
	return OK;
}

int
config_fetch_multi(char *token, struct text_line **tl)
{
	struct configuration_list *cl = CList;
	int cnt = 0;

	uppercase(token);

	while(cl) {
		if(!strcmp(cl->token, token)) {
			cnt++;
			if(cl->newvalue != NULL)
				textline_append(tl, cl->newvalue);
			else
				textline_append(tl, cl->value);
		}
		NEXT(cl);
	}
	return cnt;
}

int
config_fetch_list(char *token)
{
	struct configuration_list *cl = CList;
	int found = FALSE;

	uppercase(token);
	output[0] = 0;

	while(cl) {
		if(!strcmp(cl->token, token)) {
			sprintf(output, "%s%s\n", output, cl->value);
			found++;
		}
		NEXT(cl);
	}
	strcat(output, ".\n");
	return found;
}

char *
config_fetch(char *token)
{
	struct configuration_list *cl = CList;

	uppercase(token);

	while(cl) {
		if(!strcmp(cl->token, token)) {
			if(cl->newvalue != NULL)
				return cl->newvalue;
			return cl->value;
		}
		NEXT(cl);
	}
	return NULL;
}

char *
config_fetch_orig(char *token)
{
	struct configuration_list *cl = CList;

	uppercase(token);

	while(cl) {
		if(!strcmp(cl->token, token))
			return cl->value;
		NEXT(cl);
	}
	return NULL;
}

int
config_override(char *token, char *value)
{
	struct configuration_list *cl = CList;

	uppercase(token);

	while(cl) {
		if(!strcmp(cl->token, token)) {
			if(cl->newvalue != NULL)
				free(cl->newvalue);
			cl->newvalue = (char *)malloc(strlen(value)+1);
			strcpy(cl->newvalue, value);
			return OK;
		}
		NEXT(cl);
	}
	return ERROR;
}
