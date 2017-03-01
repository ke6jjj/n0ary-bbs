#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

int
generate_wp_update(struct active_processes *ap)
{
	char buf[80];
	struct smtp_message *g_smtpmsg, *l_smtpmsg = NULL;
	char lfn[256], gfn[256];
	FILE *lfp = NULL, *gfp;
	int gcnt, lcnt, gfd, lfd;

	strcpy(gfn, "/tmp/wpupdate.g.XXXXXX");

	if ((gfd = mkstemp(gfn)) < 0)
		return ERROR;

	if((gfp = fopen(gfn, "w")) == NULL) {
		close(gfd);
		return ERROR;
	}

	close(gfd);

	if(Wpd_Local_Distrib) {
		strcpy(lfn, "/tmp/wpupdate.l.XXXXXX");
		if ((lfd = mkstemp(lfn)) < 0) {
			fclose(gfp);
			unlink(gfn);
			return ERROR;
		}
		if ((lfp = fopen(lfn, "w")) == NULL) {
			fclose(gfp);
			unlink(gfn);
			close(lfd);
			unlink(lfn);
			return ERROR;
		}
		close(lfd);
		l_smtpmsg = malloc_struct(smtp_message);
	}

	g_smtpmsg = malloc_struct(smtp_message);

	strcpy(buf, Wpd_Global_Server);
	uppercase(buf);
	smtp_add_recipient(g_smtpmsg, buf, SMTP_REAL);

	if(l_smtpmsg != NULL) {
		strcpy(buf, Wpd_Local_Distrib);
		uppercase(buf);
		smtp_add_recipient(l_smtpmsg, buf, SMTP_REAL);
		smtp_add_sender(l_smtpmsg, Bbs_Call);
	}

	smtp_add_sender(g_smtpmsg, Bbs_Call);

	hash_gen_update(gfp, &gcnt, lfp, &lcnt);

	fclose(gfp);
	if(lfp)
		fclose(lfp);

	if(gcnt) {
		gfp = fopen(gfn, "r");
		smtp_body_from_file(g_smtpmsg, gfp);
		fclose(gfp);
		smtp_set_subject(g_smtpmsg, Wpd_Update_Subject);
		smtp_set_max_size(g_smtpmsg, Wpd_Update_Size);
		if(ap != NULL)
			disp_update(ap, g_smtpmsg);
		else
			if(fork() == 0) {
				msg_generate(g_smtpmsg);
				exit(0);
			}
	}

	smtp_free_message(g_smtpmsg);
	free(g_smtpmsg);
	unlink(gfn);


	if(l_smtpmsg != NULL) {
		if(lcnt) {
			lfp = fopen(lfn, "r");
			smtp_body_from_file(l_smtpmsg, lfp);
			fclose(lfp);
			smtp_set_subject(l_smtpmsg, Wpd_Update_Subject);
			smtp_set_max_size(l_smtpmsg, Wpd_Update_Size);
			if(ap != NULL)
				disp_update(ap, l_smtpmsg);
			else
				if(fork() == 0) {
					msg_generate(l_smtpmsg);
					exit(0);
				}
		}

		smtp_free_message(l_smtpmsg);
		free(l_smtpmsg);
		unlink(lfn);
	}
	return OK;
}
