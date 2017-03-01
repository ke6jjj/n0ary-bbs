#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "smtp.h"
#include "gateway.h"
#include "version.h"

int use_my_call = FALSE;

char
	*Gate_Spool_Dir,
	*Gate_Info_Address,
	*Bbs_Call;

struct text_line *bounce_body = NULL;

extern void
   bounce(struct smtp_message *msg, char *subject, int to_sender, int guess);

#define DETACH	1

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,		NULL },
	{ "BBS_CALL",			tSTRING,		(int*)&Bbs_Call },
	{ "BBS_HOST",			tSTRING,		(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,			(int*)&Bbsd_Port },
	{ "",					tCOMMENT,		NULL },
	{ "GATE_SPOOL_DIR",		tDIRECTORY,		(int*)&Gate_Spool_Dir },
	{ "GATE_INFO_ADDRESS",	tSTRING,		(int*)&Gate_Info_Address },
	{ NULL, 0, NULL}};

/*=======================================================================
 * This routine parses the message's From: field for the originators
 * internet address. It then opens the gateway.allow file and
 * searches for a match. The gateway.allow file looks like this:
 *
 * N0ARY	bob@arasmith.com
 * N0ARY	bob@hal.com
 * N6ZFJ	connie@arasmith.com
 * N6QMY	pat@lbc.com
 * N6QMY	pat@tandem.com
 *  and so on.......
 *
 * There can be multiple entries for each call to support them sending
 * mail from different hosts. If there is no match for the user then
 * return ERROR. A bounced mail message will be generated with info
 * on how to register.
 */


int
msg_file_generate(FILE *fp, struct smtp_message *msg)
{
	struct text_line *line;

	fprintf(fp, "%s\n", msg->from->s);
	fprintf(fp, "%s\n", msg->rcpt->s);
	fprintf(fp, "%s\n", msg->sub[0] ? msg->sub:"[no subject]");

	line = msg->header;
	while(line) {
			/* special, catch the @dir and change it to @bbs,
			 * we must hide this from the user!
			 */
		char *p = strstr(line->s, "@dir.");
		if(p != NULL) {
			*(++p) = 'b';
			*(++p) = 'b';
			*(++p) = 's';
		}
		fprintf(fp, "%s\n", line->s);
		NEXT(line);
	}
	fprintf(fp, "\n");
	return OK;
}

int
import_file_generate(FILE *fp, struct smtp_message *msg)
{
	fprintf(fp, "%s\n", msg->from->s);
	return OK;
}

int
cmd_file_generate(FILE *fp, struct smtp_message *msg)
{
	fprintf(fp, "%s\n", msg->from->s);
	fprintf(fp, "%s\n", msg->from->next->s);
	fprintf(fp, "%s\n", msg->rcpt->s);
	fprintf(fp, "%s\n", msg->sub);

	return OK;
}

static int
validate_sender(struct smtp_message *msg)
{
	int result = ERROR;
	char *q, *p, *addr;
	char from[1024];

		/* First let's do a quick check to make sure this isn't a
		 * mail bounce coming back to one of our mail bounces. Some
		 * mailers don't use Mailer-Daemon like they should. So see
		 * if it is addressed to our Mailer-Daemon.
		 */

	{
		struct text_line *l = msg->to;
		char *p, to[256];
		while(l) {
			strcpy(to, l->s);
			p = to;
			while(*p) {
				ToLower(*p);
				p++;
			}
			if(!strncmp(to, "mailer-daemon", 13))
				return TRUE;
			NEXT(l);
		}
	}

		/* search string for one of the following two patterns. We are
		 * looking for address.
		 *
		 *		address
		 *		address '(' comment ')'   <- this seen most often
		 *		comment '<' address '>'
		 *
		 * let's make an assumption that the tokens '(' and '<' can
		 * never appear in a comment.
		 */

	strcpy(from, msg->from->s);

	if((addr = (char*)index(from, '<')) == NULL) {
			/* address					form */
			/* address '(' comment ')'	form */

		q = from;
		NextChar(q);
		addr = q;
		NextSpace(q);
		*q = 0;
	} else {
		addr++;
		NextChar(addr);
		if((q = (char*)index(addr, '>')) == NULL)
			return ERROR;
		*q = 0;
	}

	textline_prepend(&(msg->from), addr);

	p = addr;
	while(*p) {
		ToLower(*p);
		p++;
	}
		/* "addr" should now point to the null terminated address */

	{
		char call[256], cmd[256];
		sprintf(cmd, "ADDRESS %s", addr);
		gated_open();
		strcpy(call, gated_fetch(cmd));
		gated_close();

		if(strncmp(call, "NO,", 3)) {
			sprintf(cmd, "TOUCH %s", addr);
			gated_open();
			gated_fetch(cmd);
			gated_close();

			result = OK;
			textline_prepend(&(msg->from), call);
			if(userd_open() == OK) {
				char ubuf[256];
				strcpy(ubuf, userd_fetch(call));
				if(strncmp(ubuf, "NO,", 3)) {
					sprintf(ubuf, "LOGIN %s", "SMTP");
					userd_fetch(ubuf);
				} else {
					struct text_line *tl = NULL;
					textline_append(&tl,
					   "Gateway user doesn't appear to have a user account");
					textline_append(&tl,
					   "anymore. This of course should not have occured.");
					textline_append(&tl,
					   "Please recreate the user account for:");
					textline_append(&tl, call);
					problem_report_textline(Bbs_Call, BBS_VERSION, call, tl);
					textline_free(tl);
				}
				userd_close();
			}
		} else {
#if 0
				/* check for special case of "@dir.arasmith.com". This is a
	 			 * protected entry that will always use the bbs call.
		 		 */
			if(strstr(msg->rcpt->s, "@dir.") != NULL)
				use_my_call = TRUE;
#endif

			if(use_my_call) {
				textline_prepend(&(msg->from), Bbs_Call);
				result = OK;
			} else {
				char *ptr = (char*)rindex(addr, '!');
				if(ptr == NULL)
					ptr = addr;
				if(!strncmp(ptr, "mailer-daemon", 13)) {
					textline_prepend(&(msg->from), "IPGATE");
					result = OK;
				}
			}
		}
	}
	return result;
}

void
build_bounce_body(char *s[])
{
	int i = 0;

	while(s[i] != NULL) {
		textline_append(&bounce_body, s[i]);
		i++;
	}
}

char *bounce_user_no_wp[] = {
 "This message could not be delivered because the user's home bbs is",
 "not known to this bbs. You will need to supply the homebbs as per",
 "the gateway instructions you received when you registered.",
 "",
 NULL };

char *bounce_info_addr[] = {
 " If you have misplaced the instructions you can get a new copy by",
 "sending a message to:",
 "", 
 NULL };

char *bounce_home_is_different[] = {
 "You have specified a home bbs different from the one in the white pages",
 "for this user.",
 "",
 "WP entry for address is shown here:",
 NULL };

char *bounce_addr_warning[] = {
 "The message is being delivered to the address you supplied.",
 "This is just a warning message.",
 "",
 NULL };

char *bounce_home_unknown[] = {
 "You have specified a home bbs you specified is not known to this",
 "bbs. This is very uncommon. Normally this only happens when the home",
 "bbs specified is not a full service bbs (not a part of the forwarding",
 "network) or is very new to the network.",
 "",
 NULL };

char *bounce_send_anyway[] = {
 "Since you have supplied a heirarchical address for this home bbs I",
 "will attempt to route the message. Be aware that the message may",
 "reach the receipient.",
 "",
 NULL };

char *bounce_reject[] = {
 "Since you have not supplied a heirarchical address this message cannot",
 "routed to the receipient. Please contact the receipient by other means",
 "and confirm his/her home bbs. Or supply the h-address as shown in the",
 "gateway instructions you received when you registered.",
 "",
 NULL };

char *bounce_hloc_not_needed[] = {
 "The home bbs you supplied on this message is known to this bbs. Therefore",
 "supplying the entire heirarchical address is not needed and can cause",
 "problems if the bbs changes his heirarchical address.",
 "",
 NULL };

static int
check_receipient(struct smtp_message *msg, char *to)
{
	char *p, home[80], cmd[256], buf[256];
	strcpy(buf, to);

	/* first, do we have a homebbs attached? If not we need to do a
	 * wp check to see if the user is known.
	 */

	if((p = strchr(buf, '@')) == NULL) {
		if(wpd_open() == ERROR)
			return OK;
		sprintf(cmd, "%s HOME", buf);
		p = wpd_fetch(cmd);
		wpd_close();
		if(!strncmp(p, "NO,", 3)) {
			build_bounce_body(bounce_user_no_wp);
			build_bounce_body(bounce_info_addr);
			textline_append(&bounce_body, Gate_Info_Address);
			textline_append(&bounce_body, "");
			return ERROR;
		}
		return OK;
	}
}

static int
parse_recipient(struct smtp_message *msg)
{
	char *p;
	char buf[256];

		/* there is a special case that ends up looking like this:
		 *   To:<@bbs.arasmith.com:wa4bbb%wb4aaa>
		 * it can be spotted by the leading '@' in the address.
		 */

	strcpy(buf, msg->rcpt->s);
	if(buf[0] == '@') {
		if((p = (char *)index(buf, ':')) != NULL) {
			char tmp[256];
			p++;
			strcpy(tmp, p);
			strcpy(buf, tmp);
		}
	} else {
		if((p = (char *)index(buf, '@')) != NULL)
			*p = 0;
	}
	if((p = (char *)index(buf, '%')) != NULL)
		*p = '@';

	p = buf;
	while(*p) {
		ToUpper(*p);
		p++;
	}

	textline_prepend(&(msg->rcpt), buf);

	if(!strncmp(buf, "MAILER", 6))
		return DROP;
	if(!strcmp(buf, "CMD"))
		return COMMAND;
	if(!strcmp(buf, "IMPORT"))
		return IMPORT;

	if(check_receipient(msg, buf) == ERROR)
		return ERROR;
	return MAIL;
}

struct smtp_message *errmsg;

void
touser(char *s)
{
	printf("%s\n", s);
	smtp_add_body(errmsg, s);
}

char *bounce_generic1[] = {
 "You must be registered to use the gateway. This is necessary to",
 "satisfy FCC regulations. For information on how to register send",
 "a message to:",
 "",
 NULL };

char *bounce_generic2[] = {
 "",
 "If you have already registered it is possible that your 'From:' field",
 "has changed and I can no longer recognize you.",
 "",
 NULL };

void
bounce(struct smtp_message *msg, char *subject, int to_sender, int guess)
{
	struct text_line *line;
	int i = 0;
	char cmd[256];

	errmsg = malloc_struct(smtp_message);

#if 0
smtp_log_enable();
#endif
	if(to_sender)
		smtp_add_recipient(errmsg, msg->from->s, SMTP_REAL);
#if 0
	smtp_add_recipient(errmsg, "Postmaster", SMTP_REAL);
#endif
	smtp_add_sender(errmsg, "Mailer-Daemon");
	smtp_set_subject(errmsg, subject);

	smtp_add_body(errmsg, "   ------ Problem with session -----");
	line = bounce_body;
	while(line != NULL) {
		smtp_add_body(errmsg, line->s);
		NEXT(line);
	}
	textline_free(bounce_body);

	if(guess == TRUE) {
		gated_open();
		sprintf(cmd, "GUESS %s", msg->from->s);
		gated_fetch_multi(cmd, touser);
		gated_close();
	}

	smtp_add_body(errmsg, "");
	smtp_add_body(errmsg, "   ------ Unsent message follows -----");

	line = msg->trash;
	while(line) {
		smtp_add_body(errmsg, line->s);
		NEXT(line);
	}

	line = msg->header;
	while(line) {
		smtp_add_body(errmsg, line->s);
		NEXT(line);
	}

	smtp_add_body(errmsg, "");

	line = msg->body;
	while(line) {
		smtp_add_body(errmsg, line->s);
		NEXT(line);
	}
	
#if 0
	smtp_add_body(errmsg, "");
	smtp_add_body(errmsg, "   ------ Debug info -----");
	line = msg->rcpt;
	while(line) {
		smtp_add_body(errmsg, line->s);
		NEXT(line);
	}
#endif
	if(smtp_send_message(errmsg) == ERROR) {
		error_log("bounce: smtp_send_message failed");
		error_print();
		smtp_print_message(errmsg);
	}
	smtp_free_message(errmsg);
	free(errmsg);
}

void
mailer(struct smtp_message *smtpmsg)
{
	struct text_line *line;
	int check = FALSE;
	char *fn;
	FILE *fp;

	bbsd_msg("in mailer");

	switch(validate_sender(smtpmsg)) {
	case OK:
		switch(parse_recipient(smtpmsg)) {
		case COMMAND:
			bbsd_msg("in mailer: COMMAND");
			if((fn = tempnam(Gate_Spool_Dir, PFX_CMD)) == NULL)
				return;
			if((fp = spool_fopen(fn)) == NULL)
				return;
			cmd_file_generate(fp, smtpmsg);
			free(fn);
			break;
		case IMPORT:
			bbsd_msg("in mailer: IMPORT");
			if((fn = tempnam(Gate_Spool_Dir, PFX_IMPORT)) == NULL)
				return;
			if((fp = spool_fopen(fn)) == NULL)
				return;
			import_file_generate(fp, smtpmsg);
			free(fn);
			break;
		case MAIL:
			bbsd_msg("in mailer: MAIL");
			if((fn = tempnam(Gate_Spool_Dir, PFX_MAIL)) == NULL)
				return;
			if((fp = spool_fopen(fn)) == NULL)
				return;
			msg_file_generate(fp, smtpmsg);
			free(fn);
			check = TRUE;
			break;
		case ERROR:
			bbsd_msg("in mailer: ERROR");
			bounce(smtpmsg, "Returned mail: Could not deliver", TRUE, FALSE);
		case DROP:
			bbsd_msg("in mailer: DROP");
			goto early_exit;
		}

		line = smtpmsg->body;
		while(line) {
			if(check && (line->s[0] == '/' || line->s[0] == '.'))
				fprintf(fp, ">%s\n", line->s);
			else
				fprintf(fp, "%s\n", line->s);
			NEXT(line);
		}
		fprintf(fp, ".");
		spool_fclose(fp);
		break;

	case TRUE:
		/* mailer-deamon is sender */
		bbsd_msg("validate_sender=TRUE build_bounce_body -1");
		build_bounce_body(bounce_generic1);
		bbsd_msg("validate_sender=TRUE textline_append");
		textline_append(&bounce_body, Gate_Info_Address);
		bbsd_msg("validate_sender=TRUE build_bounce_body -2");
		build_bounce_body(bounce_generic2);
		bbsd_msg("validate_sender=TRUE bounce");
		bounce(smtpmsg, "Returned mail: Request from unregistered user", 
			   FALSE, TRUE);
		break;

	case ERROR:
		bbsd_msg("validate_sender returned ERROR");
		build_bounce_body(bounce_generic1);
		textline_append(&bounce_body, Gate_Info_Address);
		build_bounce_body(bounce_generic2);
		bounce(smtpmsg, "Returned mail: Request from unregistered user", 
			   TRUE, TRUE);
		break;
	}

 early_exit:
	smtp_free_message(smtpmsg);
	free(smtpmsg);
	return;
}

main(int argc, char *argv[])
{
	struct smtp_message *smtpmsg;
	int retry = 60;

	if(argc > 1)
		use_my_call = TRUE;

	parse_options(argc, argv, ConfigList, "SMTPmailer");

	if(dbug_level & dbgVERBOSE)
		use_my_call = TRUE;

	while(bbsd_open(Bbs_Host, Bbsd_Port, "SMTPmail", "SMTP")) {
		bbsd_close();

		if(--retry == 0) {
			smtp_reject_connection();
			exit(0);
		}
		sleep(10);
	}
	bbsd_get_configuration(ConfigList);

	if((smtpmsg = malloc_struct(smtp_message)) == NULL) {
		smtp_reject_connection();
		exit(1);
	}

	if(smtp_recv_message(smtpmsg, mailer) != OK) {
		smtp_free_message(smtpmsg);
		free(smtpmsg);
		exit(0);
	}
	exit(0);
}

