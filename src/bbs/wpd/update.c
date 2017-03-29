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

	if (Wpd_Global_Server == NULL && Wpd_Local_Distrib == NULL)
		/* Don't send updates anywhere */
		return OK;

	/* Create temporary file for global update */
	strcpy(gfn, "/tmp/wpupdate.g.XXXXXX");
	if ((gfd = mkstemp(gfn)) < 0)
		return ERROR;
	if((gfp = fopen(gfn, "w")) == NULL) {
		close(gfd);
		return ERROR;
	}
	close(gfd);

	/* Create temporary file for local update */
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

	hash_gen_update(gfp, &gcnt, lfp, &lcnt);

	fclose(gfp);
	fclose(lfp);

	if (Wpd_Global_Server != NULL && gcnt > 0) {
		g_smtpmsg = malloc_struct(smtp_message);
		strlcpy(buf, Wpd_Global_Server, sizeof(buf));
		uppercase(buf);
		smtp_add_recipient(g_smtpmsg, buf, SMTP_REAL);
		smtp_add_sender(g_smtpmsg, Bbs_Call);
		gfp = fopen(gfn, "r");
		smtp_body_from_file(g_smtpmsg, gfp);
		fclose(gfp);
		smtp_set_subject(g_smtpmsg, Wpd_Update_Subject);
		smtp_set_max_size(g_smtpmsg, Wpd_Update_Size);
		if(ap != NULL)
			disp_update(ap, g_smtpmsg, Wpd_Global_Type);
		if(fork() == 0) {
			msg_generate(g_smtpmsg, Wpd_Global_Type);
			exit(0);
		}
		smtp_free_message(g_smtpmsg);
		free(g_smtpmsg);
	}

	unlink(gfn);

	if (Wpd_Local_Distrib != NULL && lcnt > 0) {
		l_smtpmsg = malloc_struct(smtp_message);
		strcpy(buf, Wpd_Local_Distrib);
		uppercase(buf);
		smtp_add_recipient(l_smtpmsg, buf, SMTP_REAL);
		smtp_add_sender(l_smtpmsg, Bbs_Call);
		lfp = fopen(lfn, "r");
		smtp_body_from_file(l_smtpmsg, lfp);
		fclose(lfp);
		smtp_set_subject(l_smtpmsg, Wpd_Update_Subject);
		smtp_set_max_size(l_smtpmsg, Wpd_Update_Size);
		if(ap != NULL)
			disp_update(ap, l_smtpmsg, Wpd_Local_Type);
		if(fork() == 0) {
			msg_generate(l_smtpmsg, Wpd_Local_Type);
			exit(0);
		}
		smtp_free_message(l_smtpmsg);
		free(l_smtpmsg);
	}

	unlink(lfn);

	return OK;
}
