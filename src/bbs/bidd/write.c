#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"

static void
write_comment(FILE *fp)
{
 fprintf(fp, "# This is a machine created file. If you edit it manually\n");
 fprintf(fp, "# you need to kill the bid process, edit the file, then\n");
 fprintf(fp, "# restart the bid daemon.\n");
 fprintf(fp, "#\n");

 fprintf(fp, "# This file is automatically updated once an hour if changes\n");
 fprintf(fp, "# have been made to the runtime memory image.\n");
 fprintf(fp, "#\n");

 fprintf(fp, "# The '+' character at the beginning of the line indicates\n");
 fprintf(fp, "# that the line has been preparsed so error detection can\n");
 fprintf(fp, "# be skipped. If you edit or add a line delete the '+'.\n");
 fprintf(fp, "#\n");
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

	fprintf(fp, "# v1 %s\n#\n", Bidd_File);
	write_comment(fp);
	fprintf(fp, "# Entry format, non-parsed..\n");
	fprintf(fp, "# BID SEEN\n");
	fprintf(fp, "#       |\n");
	fprintf(fp, "#       +-- mo/da/yr  (02/28/93)\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# 32405_N6QMY 02/28/93\n");
	fprintf(fp, "#\n");

	r = hash_write(fp);
	spool_fclose(fp);
	bid_image = CLEAN;

	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");
	if(dbug_level & dbgVERBOSE)
		printf("   write time = %ld seconds\n", time(NULL) - t0);
	return r;
}
