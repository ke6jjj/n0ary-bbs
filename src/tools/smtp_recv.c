#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <ctype.h>

#include "c_cmmn.h"
#include "tools.h"
#include "smtp.h"

#define CONNECT			"220 bbs Simple Mail Transfer Service Ready"
#define CONNECT_REFUSED	"421 bbs Service temporarily not available"
#define HELO_OK			"250 bbs"
#define LOCKED_OUT		"250 bbs"
#define MAIL_OK			"250 <%s>... Sender ok"
#define RCPT_OK			"250 <%s>... Recipient ok"
#define DATA_OK 		"354 Enter mail"
#define MAIL_ACCEPTED	"250 Mail accepted"
#define RSET_OK			"250 Reset state"
#define SMTP_OK			"200 OK"
#define QUIT_OK			"221 bbs delivering mail"

#define IgnoreField		0
#define FromField		1
#define SubField		2
#define RecvField		3
#define DateField		4
#define IncludeField	5
#define SpaceField		6
#define StartBody		7
#define ToField			8

static int
	smtp_parse(char *s),
	smtp_determine_field(char *str, char **p);
static char
	*smtp_find_address(char *buf),
	*smtp_line_decode(char *buf);
static void
	smtp_send(char *s),
	smtp_send1(char *s, char *param),
	smtp_recv_stdin(char *buf);

static void
smtp_recv_stdin(char *buf)
{
	char *s;
	char *p = NULL;
	while(p==NULL && !feof(stdin) && !ferror(stdin))
		p = fgets(buf, SMTP_BUF_SIZE, stdin);
	
	if((s = (char*)rindex(buf, '\r')) != NULL)
		*s = 0;
	if((s = (char*)rindex(buf, '\n')) != NULL)
		*s = 0;

	if(strlen(buf) > 1 && *buf == '.') {
		char *p = buf, *q = &buf[1];
		while(*q)
			*q++ = *p++;
	}

	smtp_incoming_log('R', buf);
}

static int
smtp_parse(char *s)
{
	if(!strncmp(s, "HELO", 4))
		return SMTP_HELO;
	if(!strncmp(s, "MAIL", 4))
		return SMTP_MAIL;
	if(!strncmp(s, "RCPT", 4))
		return SMTP_RCPT;
	if(!strncmp(s, "DATA", 4))
		return SMTP_DATA;
	if(!strncmp(s, "RSET", 4))
		return SMTP_RSET;
	if(!strncmp(s, "NOOP", 4))
		return SMTP_NOOP;
	if(!strncmp(s, "QUIT", 4))
		return SMTP_QUIT;
	return SMTP_QUIT;
}

static char *
smtp_find_address(char *buf)
{
	char *p;
	
	if((p = (char*)index(buf, '>')) == NULL)
		return NULL;
	*p = 0;

	if((p = (char*)index(buf, '<')) == NULL)
		return NULL;
	p++;
	return p;
}

static char *
smtp_line_decode(char *buf)
{
	char *p = buf;
	switch(*p) {
	case '>':
			/* decode >from here */
		break;

	case '.':
		if(*(p+1) == 0)
			return NULL;

		if(*(p+1) == '.' && *(p+2) == 0)
			*(p+2) = 0;
		break;
	}
	return buf;
}

static int
smtp_determine_field(char *str, char **p)
{
	*p = str;

	if(*str == 0)
		return StartBody;
	if(!strncmp(str, "Subject:", 8)) {
		*p = &str[8];
		NextChar(*p);
		return SubField;
	}
	if(!strncmp(str, "From:", 5)) {
		*p = &str[5];
		NextChar(*p);
		return FromField;
	}
	if(!strncmp(str, "Date:", 5)) {
		*p = &str[5];
		NextChar(*p);
		return DateField;
	}
	if(!strncmp(str, "To:", 3)) {
		*p = &str[3];
		NextChar(*p);
		return ToField;
	}
	if(!strncmp(str, "Cc:", 3)) {
		*p = &str[3];
		NextChar(*p);
		return IncludeField;
	}
	if(!strncmp(str, "Received:", 9)) {
		*p = &str[9];
		NextChar(*p);
		return RecvField;
	}
	if(isspace(*str))
		return SpaceField;
	return IgnoreField;
}


static void
smtp_send(char *s)
{
	smtp_incoming_log('S', s);
	printf("%s\r\n", s);
	fflush(stdout);
}	

static void
smtp_send1(char *s, char *param)
{
	char buf[SMTP_BUF_SIZE];

	sprintf(buf, s, param);
	smtp_incoming_log('S', buf);
	printf("%s\r\n", buf);
	fflush(stdout);
}	

void
smtp_reject_connection(void)
{
	smtp_send(CONNECT_REFUSED);
}

int
smtp_recv_message(struct smtp_message *msg, void (*handler)(struct smtp_message *smtpmsg))
{
	char buf[SMTP_BUF_SIZE];
	char *p;
	int done = FALSE;
	int in_header = TRUE;

	smtp_send(CONNECT);

	while(!done) {
		smtp_recv_stdin(buf);

		switch(smtp_parse(buf)) {
		case SMTP_HELO:
			smtp_send(HELO_OK);
			break;
		
		case SMTP_MAIL:
			p = smtp_find_address(buf);
			smtp_send1(MAIL_OK, p);
			break;

		case SMTP_RCPT:
			p = smtp_find_address(buf);
			smtp_add_recipient(msg, p, SMTP_REAL);
			smtp_send1(RCPT_OK, p);
			break;
			
		case SMTP_DATA:
			smtp_send(DATA_OK);
			while(TRUE) {
				smtp_recv_stdin(buf);
				if((p = smtp_line_decode(buf)) == NULL)
					break;

				if(in_header) {
					char *p;
					switch(smtp_determine_field(buf, &p)) {
					case ToField:
						smtp_add_recipient(msg, p, SMTP_ALIAS);
						smtp_add_header(msg, buf);
						break;
					case SubField:
						smtp_set_subject(msg, p);
						break;
					case FromField:
						smtp_add_sender(msg, p);
						smtp_add_header(msg, buf);
						break;
					case StartBody:
						in_header = FALSE;
						break;
					case DateField:
					case IncludeField:
						smtp_add_header(msg, buf);
						break;
					default:
						smtp_add_trash(msg, buf);
						break;
					}
				} else
					smtp_add_body(msg, buf);
			}
			(*handler)(msg);
			smtp_send(MAIL_ACCEPTED);
			break;

		case SMTP_RSET:
			smtp_send(RSET_OK);
			break;

		case SMTP_NOOP:
			smtp_send(SMTP_OK);
			break;
		
		case SMTP_QUIT:
			done = TRUE;
			break;
		}
	}
	return OK;
}
