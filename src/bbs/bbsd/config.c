#include <stdio.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"

struct configuration_item {
	SLIST_ENTRY(configuration_item) entries;
	char *token;
	char *value;
	char *newvalue;
};

SLIST_HEAD(configuration_list, configuration_item);

struct configuration_list ConfigurationList = SLIST_HEAD_INITIALIZER(ConfigurationList);

static void
append(char *t, char *v)
{
	struct configuration_item *cl = malloc_struct(configuration_item);

	cl->token = strdup(t);
	cl->value = strdup(v);

	SLIST_INSERT_HEAD(&ConfigurationList, cl, entries);
}

static void
port_define(char *s)
{
	char *c;
	struct Port *port;

	port = malloc_struct(Port);
	port->name = c = copy_string(get_string(&s));

	if(!strcmp(c, "STATUS") || !strcmp(c, "DAEMON"))
		port->lockable = FALSE;
	else
		port->lockable = TRUE;
	port->lock = FALSE;
	port->reason = NULL;

	SLIST_INSERT_HEAD(&PortList, port, entries);
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
	struct configuration_item *cl;
	int cnt = 0;

	uppercase(token);

	SLIST_FOREACH(cl, &ConfigurationList, entries) {
		if(!strcmp(cl->token, token)) {
			cnt++;
			if(cl->newvalue != NULL)
				textline_append(tl, cl->newvalue);
			else
				textline_append(tl, cl->value);
		}
	}
	return cnt;
}

int
config_fetch_list(char *token)
{
	struct configuration_item *cl;
	int found = FALSE;

	uppercase(token);
	output[0] = 0;

	SLIST_FOREACH(cl, &ConfigurationList, entries) {
		if(!strcmp(cl->token, token)) {
			sprintf(output, "%s%s\n", output, cl->value);
			found++;
		}
	}
	strcat(output, ".\n");
	return found;
}

char *
config_fetch(char *token)
{
	struct configuration_item *cl;

	uppercase(token);

	SLIST_FOREACH(cl, &ConfigurationList, entries) {
		if(!strcmp(cl->token, token)) {
			if(cl->newvalue != NULL)
				return cl->newvalue;
			return cl->value;
		}
	}
	return NULL;
}

char *
config_fetch_orig(char *token)
{
	struct configuration_item *cl;

	uppercase(token);

	SLIST_FOREACH(cl, &ConfigurationList, entries) {
		if(!strcmp(cl->token, token))
			return cl->value;
	}
	return NULL;
}

int
config_override(char *token, char *value)
{
	struct configuration_item *cl;

	uppercase(token);

	SLIST_FOREACH(cl, &ConfigurationList, entries) {
		if(!strcmp(cl->token, token)) {
			if(cl->newvalue != NULL)
				free(cl->newvalue);
			cl->newvalue = strdup(value);
			return OK;
		}
	}
	return ERROR;
}
