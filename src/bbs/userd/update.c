#include <stdio.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

int
scan_users(FILE *fp, time_t seen, time_t changed)
{
	struct UserDirectory *dir = UsrDir;
	time_t t = Time(NULL);
	struct tm *tm = localtime(&t);

	if(wpd_open() != OK) {
		log_error("couldn't open wpd socket");
		return ERROR;
	}

	while(dir) {
		char *p, buf[30];
		int doit = FALSE;
		sprintf(buf, "%s CHANGED", dir->call);
		p = wpd_fetch(buf);

		if(p == NULL) {
			log_error("result of wpd_fetch(%s) is NULL", buf);
			wpd_close();
			return ERROR;
		}
		
		if(strncmp(p, "NO,", 3)) {
			time_t lchanged = (time_t)get_number(&p);
			time_t lseen = (time_t)get_number(&p);

			if(lchanged > changed)
				doit = TRUE;
			else
				if(lseen > seen) {
					if(dir->refresh_day == tm->tm_wday)
						doit = TRUE;
				}
		}
		
		if(doit)
			fprintf(fp, "%s\n", p);
		NEXT(dir);
	}

	wpd_close();
	return OK;
}

int
generate_wp_update(void)
{
	char buf[80];
	struct smtp_message *smtpmsg;
	char *fn = tmpnam(NULL);
	FILE *fp = fopen(fn, "w");
	time_t seen, changed;
	time_t t0 = Time(NULL);

	if(fp == NULL)
		return ERROR;

	smtpmsg = malloc_struct(smtp_message);

	strcpy(buf, WpGlobalServer);
	uppercase(buf);
#if 1
	smtp_add_recipient(smtpmsg, "bob", SMTP_REAL);
	smtp_add_recipient(smtpmsg, buf, SMTP_ALIAS);
#else
	smtp_add_recipient(smtpmsg, buf, SMTP_REAL);
#endif
	smtp_add_sender(smtpmsg, BBS_CALL);

	seen = t0 - tDay;
	changed = t0 - WpUpdate;

	if(scan_users(fp, seen, changed) == OK) {
		fclose(fp);
		fp = fopen(fn, "r");
		smtp_body_from_file(smtpmsg, fp);
		smtp_set_subject(smtpmsg, WpUpdateSubject);
#if 1
		smtp_send_message(smtpmsg);
#else
		msg_generate(smtpmsg);
#endif
	}

	smtp_free_message(smtpmsg);
	free(smtpmsg);
	fclose(fp);
	unlink(fn);

	return OK;
}


