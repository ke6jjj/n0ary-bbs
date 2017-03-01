#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "tools.h"
#include "smtp.h"

void
smtp_body_from_file(struct smtp_message *msg, FILE *fp)
{
	char buf[1024];
	while(fgets(buf, 1024, fp)) {
		char *p = (char*)rindex(buf, '\n');
		if(p)
			*p = 0;
		if(!strcmp(buf, "/EX"))
			break;
		textline_append(&(msg->body), buf);
	}
}

void
smtp_add_recipient(struct smtp_message *msg, char *addr, int mode)
{
	textline_append(&(msg->to), addr);
	if(mode == SMTP_REAL)
		textline_append(&(msg->rcpt), addr);
}

void
smtp_add_sender(struct smtp_message *msg, char *addr)
{
	textline_append(&(msg->from), addr);
}

void
smtp_set_subject(struct smtp_message *msg, char *sub)
{
	strncpy(msg->sub, sub, 80);
}

void
smtp_set_max_size(struct smtp_message *msg, int size)
{
	msg->max_size = size;
}

void
smtp_add_body(struct smtp_message *msg, char *buf)
{
	textline_append(&(msg->body), buf);
}

void
smtp_add_header(struct smtp_message *msg, char *buf)
{
	textline_append(&(msg->header), buf);
}

void
smtp_add_trash(struct smtp_message *msg, char *buf)
{
	textline_append(&(msg->trash), buf);
}

void
smtp_add_recipient_list(struct smtp_message *msg, char *addr)
{
	char *p = addr;
	while(*p) {
		char *q = p;
		while(*p && !isspace(*p))
			p++;
		*p = 0;

		smtp_add_recipient(msg, q, SMTP_REAL);
	}
}

void
smtp_free_message(struct smtp_message *msg)
{
	msg->to = textline_free(msg->to);
	msg->from = textline_free(msg->from);
	msg->rcpt = textline_free(msg->rcpt);
	msg->body = textline_free(msg->body);
	msg->header = textline_free(msg->header);
	msg->trash = textline_free(msg->trash);
	msg->sub[0] = 0;
}

void
smtp_print_message(struct smtp_message *msg)
{
	struct text_line *tl;

	printf("SUBJECT -------------------------------------------------\n");
	printf("%s\n", msg->sub);
	printf("TO ------------------------------------------------------\n");
	tl = msg->to;
	while(tl) {
		printf("%s\n", tl->s);
		NEXT(tl);
	}
	printf("FROM ----------------------------------------------------\n");
	tl = msg->from;
	while(tl) {
		printf("%s\n", tl->s);
		NEXT(tl);
	}
	printf("RCPT ----------------------------------------------------\n");
	tl = msg->rcpt;
	while(tl) {
		printf("%s\n", tl->s);
		NEXT(tl);
	}
	printf("BODY ----------------------------------------------------\n");
	tl = msg->body;
	while(tl) {
		printf("%s\n", tl->s);
		NEXT(tl);
	}
	printf("HEADER --------------------------------------------------\n");
	tl = msg->header;
	while(tl) {
		printf("%s\n", tl->s);
		NEXT(tl);
	}
	printf("TRASH ---------------------------------------------------\n");
	tl = msg->trash;
	while(tl) {
		printf("%s\n", tl->s);
		NEXT(tl);
	}
	printf("DONE ----------------------------------------------------\n");
}

