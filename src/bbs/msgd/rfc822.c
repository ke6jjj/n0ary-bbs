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
	const char *heldby_s = rfc822_xlate(rHELDBY);
	const char *heldwhy_s = rfc822_xlate(rHELDWHY);
	size_t heldby = strlen(heldby_s);
	size_t heldwhy = strlen(heldwhy_s);
	int nl, next_nl, out;
	size_t len;

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);

	for (nl = TRUE, out = 0; fgets(buf, sizeof(buf), fp); nl = next_nl) {
		len = strlen(buf);
		next_nl = buf[len-1] == '\n';

		if (out) {
			/* We're outputing a long line */
			socket_raw_write(ap->fd, buf);
			/* Keep writing until newline is found */
			out = !next_nl;
			continue;
		}
			
		if(nl && !strncmp(buf, heldby_s, heldby)) {
			snprintf(output, sizeof(output), "By: %s",
				&buf[heldby+1]);
			socket_raw_write(ap->fd, output);
			if (!next_nl) {
				/* Super long line, keep printing */
				out = TRUE;
			}
			continue;
		}
		if(nl && !strncmp(buf, heldwhy_s, heldwhy)) {
			snprintf(output, sizeof(output), "%s",
				&buf[heldwhy+1]);
			socket_raw_write(ap->fd, output);
			if (!next_nl) {
				/* Super long line, keep printing */
				out = TRUE;
			}
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
	const char *token_s = rfc822_xlate(token);
	size_t toklen = strlen(token_s);
	size_t len;
	int nl, next_nl;

	if(fp == NULL)
		return NULL;

	/* Ensure that the output begins empty */
	output[0] = '\0';

	rfc822_skip_to(fp);

	for (nl = TRUE; fgets(buf, sizeof(buf), fp); nl = next_nl) {
		len = strlen(buf);
		next_nl = buf[len-1] == '\n';

		if(nl && !strncmp(buf, token_s, toklen))
			strlcpy(output, &buf[toklen+1], sizeof(output));
	}

	close_message(fp);

	return output;
}

int
rfc822_skip_to(FILE *fp)
{
	char buf[1024];
	size_t len;
	int nl, next_nl;

	for (nl = TRUE; fgets(buf, sizeof(buf), fp); nl = next_nl) {
		len = strlen(buf);
		next_nl = buf[len-1] == '\n';

		if(nl && !strcmp(buf, "/EX\n"))
			return OK;
	}
	return ERROR;
}

int
rfc822_decode_fields(struct msg_dir_entry *m)
{
	FILE *fp = open_message(m->number);
	char buf[1024];
	int nothing = TRUE;
	size_t len;
	int nl, next_nl;

	if(fp == NULL)
		return ERROR;

	rfc822_skip_to(fp);
	m->size = ftell(fp);

	for (nl = TRUE; fgets(buf, sizeof(buf), fp); nl = next_nl) {
		len = strlen(buf);
		next_nl = buf[len-1] == '\n';

		if (next_nl)
			/* Remove newline from end */
			buf[len-1] = '\0';

		nothing = FALSE;
		if (nl)
			rfc822_parse(m, buf);
	}
	close_message(fp);

	if(read_by_rcpt(m))
		m->flags |= MsgRead;

	if(nothing)
		return ERROR;
	return OK;
}

