#include <stdio.h>
#include <dirent.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "gateway.h"

extern int
	msg_generate(FILE *fp),
	cmd_generate(FILE *fp),
	import_generate(FILE *fp);

char 
	*Bin_Dir,
	*Gate_Personal_File,
	*Gate_Spool_Dir;

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,		NULL },
	{ "BBS_HOST",			tSTRING,		(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,			(int*)&Bbsd_Port },
	{ "",					tCOMMENT,		NULL },
	{ "GATE_SPOOL_DIR",		tDIRECTORY,		(int*)&Gate_Spool_Dir },
	{ "BBS_PERSONAL_FILE",	tFILE,			(int*)&Gate_Personal_File },
	{ "BIN_DIR",			tDIRECTORY,		(int*)&Bin_Dir },
	{ NULL, 0, NULL}};

static int
filetype(FILE **fp, char *root, char *fn)
{
	int type = ERROR;
	int c;

	if(!strncmp(root, PFX_MAIL, strlen(PFX_MAIL)))
		type = MAIL;
	else {
		if(!strncmp(root, PFX_CMD, strlen(PFX_CMD)))
			type = COMMAND;
		else {
			if(!strncmp(root, PFX_IMPORT, strlen(PFX_IMPORT)))
				type = IMPORT;
			else
				return ERROR;
		}
	}

	if((*fp = fopen(fn, "r")) == NULL) {
		perror("filetype(open)");
		return ERROR;
	}

	if(fseek(*fp, (-1), 2)) {
		perror("filetype(fseek)");
		return ERROR;
	}

	if((c = fgetc(*fp)) == EOF) {
		perror("filetype(fgetc)");
		return ERROR;
	}

			/* if I saved the file with vi then there will "\n.\n" */

	if(c == '\n') {
		if(fseek(*fp, (-3), 2)) {
			perror("filetype(fseek)");
			return ERROR;
		}

		if((c = fgetc(*fp)) == EOF) {
			perror("filetype(fgetc)");
			return ERROR;
		}

		if(c != '\n') {
			return ERROR;
		}

		if((c = fgetc(*fp)) == EOF) {
			perror("filetype(fgetc)");
			return ERROR;
		}
	}

	if(c == '.') {
		if(fseek(*fp, 0, 0)) {
			perror("filetype(fseek)");
			return ERROR;
		}

		return type;
	}

	return ERROR;
}

void
main(int argc, char *argv[])
{
	struct dirent *dp;
	DIR *dirp;

	parse_options(argc, argv, ConfigList, "Gateway");

	if(bbsd_open(Bbs_Host, Bbsd_Port, "QUEUE", "SMTP"))
		exit(0);
	bbsd_get_configuration(ConfigList);

			/* check to see if I am the only one. If I'm not then
			 * I should exit.
			 */

	if(bbsd_check("QUEUE", NULL) != 1) {
		bbsd_msg("Already running");
		exit(0);
	}

	dirp = opendir(Gate_Spool_Dir);
	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
		FILE *fp = (FILE *)NULL;
		char fn[256], nfn[256];
		int result = ERROR;

		sprintf(fn, "%s/%s", Gate_Spool_Dir, dp->d_name);
		bbsd_msg(dp->d_name);
		switch(filetype(&fp, dp->d_name, fn)) {
		case IMPORT:
			result = import_generate(fp);
			break;
		case COMMAND:
			result = cmd_generate(fp);
			break;
		case MAIL:
			result = msg_generate(fp);
			break;
		default:
			if(fp)
				fclose(fp);
			continue;
		}
		fclose(fp);
		switch(result) {
		case OK:
			unlink(fn);
			break;
		case TRUE:
			sprintf(nfn, "%s/z%s", Gate_Spool_Dir, dp->d_name);
			rename(fn, nfn);
			break;
		}
	}
	closedir(dirp);
	bbsd_msg("Done");
	bbsd_close();
	exit(0);
}
