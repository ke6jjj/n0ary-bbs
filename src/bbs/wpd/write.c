#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

static void
write_comment(FILE *fp)
{
 fprintf(fp, "# This is a machine created file. If you edit it manually\n");
 fprintf(fp, "# you need to kill the wpd process, edit the file, then\n");
 fprintf(fp, "# restart the wpd daemon.\n");
 fprintf(fp, "#\n");

 fprintf(fp, "# This file is automatically updated once an hour if changes\n");
 fprintf(fp, "# have been made to the runtime memory image.\n");
 fprintf(fp, "#\n");

 fprintf(fp, "# The '+' character at the beginning of the line indicates\n");
 fprintf(fp, "# that the line has been preparsed so error detection can\n");
 fprintf(fp, "# be skipped. If you edit or add a line delete the '+'.\n");
 fprintf(fp, "#\n");
}

static char *
write_full_file()
{
	FILE *fp = spool_fopen(Wpd_Dump_File);
	char *r;

	if(fp == NULL)
		return "Failed to open output file\n";

	r = hash_write(fp, WriteALL);
	spool_fclose(fp);
	return r;
}

char *
write_user_file(void)
{
	FILE *fp = spool_fopen(Wpd_User_File);
	char *r;

	time_t t0;
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(fp == NULL)
		return "Failed to open output file\n";

	fprintf(fp, "# v1 %s\n#\n", Wpd_User_File);
	write_comment(fp);
	fprintf(fp, "# Entry format, non-parsed..\n");
	fprintf(fp, "# CALL LEVEL FIRSTSEEN SEEN CHANGED UPDATED HOMEBBS FNAME ZIP QTH\n");
	fprintf(fp, "#        |       |      |      |       |\n");
	fprintf(fp, "#        |       +------+------+-------+- mo/da/yr  (02/28/93)\n");
	fprintf(fp, "#        +- 0 = Sysop\n");
	fprintf(fp, "#        +- 1 = User\n");
	fprintf(fp, "#        +- 2 = Questionable\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# N6ZFJ 1 12/14/91 02/01/93 01/15/93 01/01/85 N0ARY Connie 94086 Sunnyvale, CA\n");
	fprintf(fp, "#\n");

	r = hash_write(fp, WriteUSER);
	spool_fclose(fp);
	user_image = CLEAN;
	if(dbug_level & dbgVERBOSE)
		printf("Writing user file took %ld seconds\n", time(NULL) - t0);
	return r;
}

char *
write_bbs_file(void)
{
	FILE *fp = spool_fopen(Wpd_Bbs_File);
	char *r;

	time_t t0;
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(fp == NULL)
		return "Failed to open output file\n";

	fprintf(fp, "# v1 %s\n#\n", Wpd_Bbs_File);
	write_comment(fp);
	fprintf(fp, "# Entry format, non-parsed..\n");
	fprintf(fp, "# BBSCALL LEVEL HLOC\n");
	fprintf(fp, "#           |\n");
	fprintf(fp, "#           +- 0 = Sysop\n");
	fprintf(fp, "#           +- 1 = User\n");
	fprintf(fp, "#           +- 2 = Questionable\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# N0ARY 1 #NOCAL.CA.USA.NA\n");
	fprintf(fp, "#\n");

	r = hash_write(fp, WriteBBS);
	spool_fclose(fp);
	bbs_image = CLEAN;
	if(dbug_level & dbgVERBOSE)
		printf("Writing bbs file took %ld seconds\n", time(NULL) - t0);
	return r;
}

/*
	WRITE
	WRITE USER
	WRITE BBS
*/

char *
write_file(char *cmd)
{
	uppercase(cmd);

	if(!strncmp(cmd, "USER", 4))
		return write_user_file();

	if(!strncmp(cmd, "BBS", 3))
		return write_bbs_file();

	return write_full_file();
}

