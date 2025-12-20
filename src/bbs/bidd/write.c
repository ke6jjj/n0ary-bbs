#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"

static int
write_comment(FILE *fp)
{
    return fputs(
        "# This is a machine created file. If you edit it manually\n"
        "# you need to kill the bid process, edit the file, then\n"
        "# restart the bid daemon.\n"
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
write_bid_help(FILE *fp)
{
    return fputs(
        "# Entry format, non-parsed..\n"
        "# BID SEEN\n"
        "#       |\n"
        "#       +-- mo/da/yr  (02/28/93)\n"
        "#\n"
        "# 32405_N6QMY 02/28/93\n"
        "#\n",
	fp
    );
}

char *
write_file()
{
	FILE *fp = spool_fopen(Bidd_File);
	char *r;
	time_t t0;
	
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(fp == NULL)
		return "Failed to open output file\n";
	
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("Flushing");

	if (fprintf(fp, "# v1 %s\n#\n", Bidd_File) < 0)
		goto WriteFailed;
	if (write_comment(fp) < 0)
		goto WriteFailed;
	if (write_bid_help(fp) < 0)
		goto WriteFailed;
	if (hash_write(fp) < 0)
		goto WriteFailed;

	if (spool_fclose(fp) < 0)
		goto CloseFailed;

	bid_image = CLEAN;

	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");
	if(dbug_level & dbgVERBOSE)
		printf("   write time = %"PRTMd" seconds\n", time(NULL) - t0);

	return "OK\n";

WriteFailed:
	spool_abort(fp);
CloseFailed:
	return "ERROR\n";
}
