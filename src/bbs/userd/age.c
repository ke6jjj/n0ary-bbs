#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

static int
homebbs(char *call)
{
FILE *fp = fopen("/tmp/userd", "a+");
	char *c, cmd[80];

	sprintf(cmd, "%s HOME", call);
	c = wpd_fetch(cmd);
fprintf(fp, "*** call:%s home:%s me:%s\n", call, c, Bbs_Call);
fclose(fp);
	if(!strcmp(c, Bbs_Call))
		return TRUE;
	return FALSE;
}

/*ARGSUSED*/
void
dummy(char *s)
{
	return;
}

char *
age_users(struct active_processes *ap)
{
	struct UserDirectory *dir;
	time_t now = Time(NULL);
	time_t suspect_t = now - Userd_Age_Suspect;
	time_t home_t = now - Userd_Age_Home;
	time_t nonhome_t = now - Userd_Age_NotHome;
	time_t nonham_t = now - Userd_Age_NonHam;

	int suspect_cnt = 0;
	int user_cnt = 0;
	int nonham_cnt = 0;
	FILE *dfp;
	char reason[80];
	
	if(dbug_level)
		dfp = fopen("aging.out", "w");

	wpd_open();
	gated_open();

	LIST_FOREACH(dir, &UsrDir, entries) {
		char call[10];

		if(dir->immune) {
			continue;
		}

		call[0] = 0;
		if(dir->class == CALLisSUSPECT) {
			if(dir->lastseen < suspect_t) {
				strcpy(call, dir->call);

				if(ap)
					sprintf(reason, "%s, suspect [%"PRTMd" days]",
						call, (now - dir->lastseen)/tDay);

				if(dbug_level) {
					fprintf(dfp, "age user %s, suspect call [%"PRTMd" days]\n",
						call, (now - dir->lastseen)/tDay);
					suspect_cnt++;
				}
			}
		} else
			if(dir->class == CALLisNAME) {
				if(dir->lastseen < nonham_t) {
					strcpy(call, dir->call);

					if(ap)
						sprintf(reason, "%s, non-ham [%"PRTMd" days]",
							call, (now - dir->lastseen)/tDay);

					if(dbug_level) {
						fprintf(dfp, "age user %s, non-ham [%"PRTMd" days]\n",
							call, (now - dir->lastseen)/tDay);
						nonham_cnt++;
					}
				}
			} else
				if(dir->lastseen < nonhome_t) {
					int home_here = homebbs(dir->call);	
					time_t t = home_here ? home_t : nonhome_t;
					if(dir->lastseen < t) {
						strcpy(call, dir->call);

						if(ap)
							sprintf(reason, "%s%s, age [%"PRTMd" days]",
								call, home_here?"@HERE":"",
								(now - dir->lastseen)/tDay);

						if(dbug_level) {
							fprintf(dfp, "age user %s [%"PRTMd" days]\n",
								call, (now - dir->lastseen)/tDay);
							user_cnt++;
						}
					}
				}

		
		if(call[0]) {
			char cmd[80];
			sprintf(cmd, "USER %s", call);
			if(gated_fetch_multi(cmd, dummy) == ERROR) {
				if(ap) strcat(reason, "... KILLED");
				kill_user(call);
			}

			if(ap) {
				socket_write(ap->fd, reason);
			}
		}
	}
	if(dbug_level) {
		fclose(dfp);
		printf("suspects = %d\nnon-ham = %d\nusers = %d\n", suspect_cnt,
			nonham_cnt, user_cnt);
	}

	wpd_close();
	gated_close();
	return "OK\n";
}
