#include <stdio.h>
#include <limits.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "msgd.h"

FILE *
open_message(int num)
{
	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "%s/%05d", Msgd_Body_Path, num);
	return fopen(filename, "r+");
}

FILE *
open_message_write(int num)
{
	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "%s/%05d", Msgd_Body_Path, num);
	return fopen(filename, "w");
}

void
close_message(FILE *fp)
{
	fclose(fp);
}

int
rfc822_append_tl(int num, int token, struct text_line *tl)
{
	FILE *fp = open_message(num);

	if(fp == NULL)
		return ERROR;

	fseek(fp, 0, 2);
	while(tl) {
		fprintf(fp, "%s %s\n", rfc822_xlate(token), tl->s);
		NEXT(tl);
	}
	close_message(fp);
	return OK;
}

int
rfc822_append_complete(int num, char *s)
{
	FILE *fp = open_message(num);
	if(fp == NULL)
		return ERROR;

	fseek(fp, 0, 2);
	fprintf(fp, "%s\n", s);
	close_message(fp);
	return OK;
}

int
rfc822_append(int num, int token, char *s)
{
	char buf[80];
	snprintf(buf, sizeof(buf), "%s %s", rfc822_xlate(token), s);
	return rfc822_append_complete(num, buf);
}

int
rfc822_display_held(struct active_processes *ap, int num)
{
	FILE *fp = open_message(num);
	char buf[1024];
	int heldby = strlen(rfc822_xlate(rHELDBY));
	int heldwhy = strlen(rfc822_xlate(rHELDWHY));

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);
	while(fgets(buf, sizeof(buf), fp)) {
		if(!strncmp(buf, rfc822_xlate(rHELDBY), heldby)) {
			snprintf(output, sizeof(output), "By: %s",
				&buf[strlen(rfc822_xlate(rHELDBY))+1]);
			socket_raw_write(ap->fd, output);
			continue;
		}
		if(!strncmp(buf, rfc822_xlate(rHELDWHY), heldwhy)) {
			snprintf(output, sizeof(output), "%s",
				&buf[strlen(rfc822_xlate(rHELDWHY))+1]);
			socket_raw_write(ap->fd, output);
			continue;
		}
	}
	close_message(fp);
	return OK;
}

char *
rfc822_get_field(int num, int token)
{
	FILE *fp = open_message(num);
	char buf[1024];
	static char output[1024];
	int len = strlen(rfc822_xlate(token));

	if(fp == NULL)
		return NULL;

	/* Ensure that the output begins empty */
	output[0] = '\0';

	rfc822_skip_to(fp);
	while(fgets(buf, sizeof(buf), fp))
		if(!strncmp(buf, rfc822_xlate(token), len))
			strlcpy(output, &buf[strlen(rfc822_xlate(token))+1], sizeof(output));

	close_message(fp);

	return output;
}

int
rfc822_skip_to(FILE *fp)
{
	char buf[1024];

	while(fgets(buf, sizeof(buf), fp))
		if(!strcmp(buf, "/EX\n"))
			return OK;
	return ERROR;
}

int
rfc822_decode_fields(struct msg_dir_entry *m)
{
	FILE *fp = open_message(m->number);
	char buf[1024];
	int nothing = TRUE;

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);
	m->size = ftell(fp);
	while(fgets(buf, sizeof(buf), fp)) {
		if (buf[0] != '\0')
			buf[strlen(buf)-1] = 0;

		nothing = FALSE;
		rfc822_parse(m, buf);
	}
	close_message(fp);

	if(read_by_rcpt(m))
		m->flags |= MsgRead;

	if(nothing)
		return ERROR;
	return OK;
	
}

