#include <stdio.h>

#define SMTP_HELO	1
#define SMTP_MAIL	2
#define SMTP_RCPT	3
#define SMTP_DATA	4
#define SMTP_RSET	5
#define SMTP_NOOP	6
#define SMTP_QUIT	7

#define SMTP_REAL		1
#define SMTP_ALIAS		0

#define SMTP_BUF_SIZE	4096

struct smtp_message {
	struct text_line *to, *from, *rcpt;
	char sub[80];
	struct text_line *header;
	struct text_line *body;
	struct text_line *trash;
	int max_size;
};

/* smtp_msg.c */

extern void
	smtp_log_enable(void),
	smtp_log_disable(void),
	smtp_outgoing_log(char c, char *s),
	smtp_incoming_log(char c, char *s),
	smtp_body_from_file(struct smtp_message *msg, FILE *fp),
	smtp_add_recipient(struct smtp_message *msg, char *addr, int mode),
	smtp_add_sender(struct smtp_message *msg, char *addr),
	smtp_set_subject(struct smtp_message *msg, char *sub),
	smtp_set_max_size(struct smtp_message *msg, int size),
	smtp_add_body(struct smtp_message *msg, char *buf),
	smtp_add_header(struct smtp_message *msg, char *buf),
	smtp_add_trash(struct smtp_message *msg, char *buf),
	smtp_add_recipient_list(struct smtp_message *msg, char *addr),
	smtp_print_message(struct smtp_message *msg),
	smtp_free_message(struct smtp_message *msg);

/* smtp_send.c */

extern void
	smtp_set_port(int port);

extern int
	smtp_send_message(struct smtp_message *msg);


/* smtp_recv.c */

extern void
	smtp_reject_connection(void);

extern int
	smtp_recv_message(struct smtp_message *msg, void (*handler)());
