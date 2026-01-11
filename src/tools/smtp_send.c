#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"
#include "smtp.h"

#ifdef MAIL_HOST
static char *mailhost = MAIL_HOST;
#else
static char *mailhost = "mailhost";
#endif

static int
	smtp_port_number = 25,
	smtp_disconnect(int fd),
	smtp_request(int fd, char *s),
	smtp_recv_ack(int fd);

static void
	smtp_data(int fd, char *s);

static int
smtp_disconnect(int fd)
{
	close(fd);
	return ERROR;
}

static void
smtp_escape_period(int fd)
{
	smtp_outgoing_log('S', ".");
	write(fd, ".", 1);
}


static void
smtp_data(int fd, char *s)
{
	char buf[SMTP_BUF_SIZE];
	char *p = (char*)index(s, '\n');
	if(p)
		*p = 0;

	smtp_outgoing_log('S', s);
	sprintf(buf, "%s\r\n", s);
	write(fd, buf, strlen(buf));
}

static int
smtp_recv_ack(int fd)
{
	int fdlimit = fd + 1;
	char buf[SMTP_BUF_SIZE];
	struct timeval t;
	int cnt;
	fd_set ready;

	t.tv_sec = 360;
	t.tv_usec = 0;

	FD_ZERO(&ready);
	FD_SET(fd, &ready);

	if((cnt=select(fdlimit, (void*)&ready, 0, 0, &t)) < 0) {
		perror("select");
		exit(9);
	}

	if(cnt == 0) {
		printf("smtp_recv_ack(360sec): timeout occured\n");
		return ERROR;
	}

	if(FD_ISSET(fd, &ready)) {
		int len = read(fd, buf, SMTP_BUF_SIZE);
		buf[len] = 0;
		switch(buf[0]) {
		case '2':
		case '3':
			smtp_outgoing_log('R', buf);
			return OK;
		}
	}

	return ERROR;
}

static int
smtp_request(int fd, char *s)
{
	smtp_data(fd, s);
	return smtp_recv_ack(fd);
}


int
smtp_send_message(struct smtp_message *msg)
{
	extern char *bbsd_get_variable(char *);
	struct text_line *body = msg->body;
	struct text_line *header = msg->header;
	char bbs_domain[256];
	char bbs_call[80];
	char buf[80];
	struct text_line *tl;
	char buffer[4096];
	int fd = ERROR;
	int csize = 0;
	char *err = NULL;

	
	strcpy(bbs_domain, bbsd_get_variable("BBS_DOMAIN"));
	strcpy(bbs_call, bbsd_get_variable("BBS_CALL"));

	strcpy(buf, bbsd_get_variable("SMTP_PORT"));
	if(strncmp(buf, "NO,", 3))
		smtp_port_number = atoi(buf);

	strcpy(buf, bbsd_get_variable("MAIL_HOST"));
	if(strncmp(buf, "NO,", 3))
		mailhost = buf;

	if((fd = socket_open(mailhost, smtp_port_number)) == ERROR) {
		log_error("smtp_send_message: error opening smtp socket");
		return ERROR;
	}

	if(smtp_recv_ack(fd) != OK) {
		log_error("smtp_send_message: target did not id properly");
		fd = smtp_disconnect(fd);
		return ERROR;
	}
	
	sprintf(buffer, "HELO %s", bbs_domain);
	if(smtp_request(fd, buffer) != OK) {
		log_error("smtp_send_message: HELO exchange failed");
		fd = smtp_disconnect(fd);
		return ERROR;
	}

	sprintf(buffer, "MAIL FROM:<%s@%s>", msg->from->s, bbs_domain);
	if(smtp_request(fd, buffer) != OK) {
		log_error("smtp_send_message: \"%s\" failed", buffer);
		fd = smtp_disconnect(fd);
		return ERROR;
	}
	
	tl = msg->rcpt;
	if(tl == NULL) {
		sprintf(buffer, "RCPT TO:<postmaster>");
		if(smtp_request(fd, buffer) != OK) {
			smtp_set_subject(msg, "BBS: No RCPT specified");
			log_error("smtp_send_message: \"%s\" failed", buffer);
			fd = smtp_disconnect(fd);
			return ERROR;
		}
	}
	while(tl != NULL) {
		sprintf(buffer, "RCPT TO:<%s>", tl->s);
		if(smtp_request(fd, buffer) != OK) {
			log_error("smtp_send_message: \"%s\" failed", buffer);
			fd = smtp_disconnect(fd);
			return ERROR;
		}
		NEXT(tl);
	}

	if(smtp_request(fd, "DATA") != OK) {
		log_error("smtp_send_message: DATA failed");
		fd = smtp_disconnect(fd);
		return ERROR;
	}

    while(header != NULL) {
		if(*(header->s) == '.') {
			smtp_escape_period(fd);
			csize += 1;
		}

		smtp_data(fd, header->s);
		NEXT(header);
	}

	sprintf(buffer, "From: %s@%s", msg->from->s, bbs_domain);
	smtp_data(fd, buffer);

	tl = msg->to;
	if (tl != NULL) {
		sprintf(buffer, "To: %s", tl->s);
		smtp_data(fd, buffer);
	}

	sprintf(buffer, "Subject: %s", msg->sub);
	smtp_data(fd, buffer);
	sprintf(buffer, "Comment: via %s/BBS gateway", bbs_call);
	smtp_data(fd, buffer);
	smtp_data(fd, "");


	while(body != NULL) {
		if(*(body->s) == '.') {
			smtp_escape_period(fd);
			csize += 1;
		}
		smtp_data(fd, body->s);
		csize += strlen(body->s);
		NEXT(body);

		if(msg->max_size != 0)
			if(csize > msg->max_size)
				break;
	}

	if(smtp_request(fd, ".") != OK) {
		log_error("smtp_send_message: DATA END failed");
		fd = smtp_disconnect(fd);
		return ERROR;
	}

	if(smtp_request(fd, "QUIT") != OK) {
		log_error("smtp_send_message: QUIT failed");
		fd = smtp_disconnect(fd);
		return ERROR;
	}

	fd = smtp_disconnect(fd);
	return OK;
}

void
smtp_set_port(int port)
{
	if(port != 0)
		smtp_port_number = port;
}
