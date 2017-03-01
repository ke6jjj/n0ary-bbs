#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

char *Log_Dir = NULL;

static void
log_setdir(void)
{
	char *p;

	if(Log_Dir != NULL)
		return;

	if((p = bbsd_get_variable("LOGD_DIR")) != NULL) {
		char *dir = copy_string(p);
		p = bbsd_get_variable("BBS_DIR");

		Log_Dir = malloc(strlen(p)+strlen(dir)+2);
		sprintf(Log_Dir, "%s/%s", p, dir);
		free(dir);
	}
}

void
log_f(char *name, char *s, char *p)
{
	FILE *fp;
	char fn[80];

	if(Logging == logOFF)
		return;

	log_setdir();
	sprintf(fn, "%s/%s", Log_Dir, name);
	if((fp = fopen(fn, "a+")) != NULL) {
		fprintf(fp, "%s%s\n", s, p);
		fclose(fp);
	}
}

void
log_clear(char *name)
{
	char fn[80];

	if(Logging == logONnCLR) {
		sprintf(fn, "%s/%s", Log_Dir, name);
		unlink(fn);
	}
}
