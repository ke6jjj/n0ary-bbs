#include <stdio.h>
#include "c_cmmn.h"
#include "tools.h"

#define RECV_LOG_FILE	"/tmp/incoming"
#define SEND_LOG_FILE	"/tmp/outgoing"

static int smtp_log_enabled = FALSE;

smtp_log_enable(void)
{
	smtp_log_enabled = TRUE;
}

smtp_log_disable(void)
{
	smtp_log_enabled = FALSE;
}

static void
smtp_log(char *fn, char c, char *s)
{
	FILE *fp;

	if((fp = fopen(fn, "a")) != NULL) {
		fprintf(fp, "%c: %s\n", c, s);
		fclose(fp);
	}
}

void
smtp_outgoing_log(char c, char *s)
{
	if(smtp_log_enabled == TRUE)
		smtp_log(SEND_LOG_FILE, c, s);
}

void
smtp_incoming_log(char c, char *s)
{
	if(smtp_log_enabled == TRUE)
		smtp_log(RECV_LOG_FILE, c, s);
}
