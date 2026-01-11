#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "wp.h"

static int
write_comment(FILE *fp)
{
    return fputs(
        "# This is a machine created file. If you edit it manually\n"
        "# you need to kill the wpd process, edit the file, then\n"
        "# restart the wpd daemon.\n"
        "#\n"

        "# This file is automatically updated once an hour if changes\n"
        "# have been made to the runtime memory image.\n"
        "#\n"

        "# The '+' character at the beginning of the line indicates\n"
        "# that the line has been preparsed so error detection can\n"
        "# be skipped. If you edit or add a line delete the '+'.\n"
        "#\n",
        fp
    );
}

static int
write_users_help(FILE *fp)
{
    return fputs(
        "# Entry format, non-parsed..\n"
        "# CALL LEVEL FIRSTSEEN SEEN CHANGED UPDATED HOMEBBS FNAME ZIP QTH\n"
        "#        |       |      |      |       |\n"
        "#        |       +------+------+-------+- mo/da/yr  (02/28/93)\n"
        "#        +- 0 = Sysop\n"
        "#        +- 1 = User\n"
        "#        +- 2 = Questionable\n"
        "#\n"
        "# N6ZFJ 1 12/14/91 02/01/93 01/15/93 01/01/85 N0ARY Connie 94086 Sunnyvale, CA\n"
        "#\n",
        fp
    );
}

static int
write_bbs_help(FILE *fp)
{
    return fputs(
        "# Entry format, non-parsed..\n"
        "# BBSCALL LEVEL HLOC\n"
        "#           |\n"
        "#           +- 0 = Sysop\n"
        "#           +- 1 = User\n"
        "#           +- 2 = Questionable\n"
        "#\n"
        "# N0ARY 1 #NOCAL.CA.USA.NA\n"
        "#\n",
        fp
    );
}

static char *
write_full_file()
{
	FILE *fp = spool_fopen(Wpd_Dump_File);
	int r;

	if(fp == NULL)
		return "Failed to open output file\n";

	if (hash_write(fp, WriteALL) < 0)
		goto WriteFailed;
	if (spool_fclose(fp) < 0)
		goto CloseError;

	return "OK\n";

WriteFailed:
	spool_abort(fp);
CloseError:
	return "ERROR\n";
}

char *
write_user_file(void)
{
	FILE *fp = spool_fopen(Wpd_User_File);
	int r;

	time_t t0;
	t0 = time(NULL);

	if(fp == NULL)
		return "Failed to open output file\n";

	if (fprintf(fp, "# v1 %s\n#\n", Wpd_User_File) < 0)
		goto WriteError;

	if (write_comment(fp) < 0)
		goto WriteError;

	if (write_users_help(fp) < 0)
		goto WriteError;

	if (hash_write(fp, WriteUSER) < 0)
		goto WriteError;
;
	if (spool_fclose(fp) < 0)
		goto CloseError;

	user_image = CLEAN;
	log_debug("Writing user file took %"PRTMd" seconds", time(NULL) - t0);
	return "OK\n";

WriteError:
	spool_abort(fp);
CloseError:
	return "ERROR\n";
}

char *
write_bbs_file(void)
{
	FILE *fp = spool_fopen(Wpd_Bbs_File);
	char *r;

	time_t t0;
	t0 = time(NULL);

	if(fp == NULL)
		return "Failed to open output file\n";

	if (fprintf(fp, "# v1 %s\n#\n", Wpd_Bbs_File) < 0)
		goto WriteError;
	if (write_comment(fp) < 0)
		goto WriteError;
	if (write_bbs_help(fp) < 0)
		goto WriteError;
	if (hash_write(fp, WriteBBS) < 0)
		goto WriteError;
	if (spool_fclose(fp) < 0)
		goto CloseError;

	bbs_image = CLEAN;
	log_debug("Writing bbs file took %"PRTMd" seconds", time(NULL) - t0);
	return "OK\n";

WriteError:
	spool_abort(fp);
CloseError:
	return "ERROR\n";
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

