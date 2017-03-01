#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "tools.h"

static 
struct OpenFiles {
	struct OpenFiles *next;
	char *tmpname;
	char *name;
	FILE *fp;
} *open_file = NULL;

FILE *
spool_fopen(char *fn)
{
	static int number = 0;
	struct OpenFiles *f = malloc_struct(OpenFiles);

	f->name = (char*)malloc(strlen(fn)+1);
	f->tmpname = (char*)malloc(strlen(fn)+20);
	strcpy(f->name, fn);
	sprintf(f->tmpname, "%s.%d.%d", fn, getpid(), number++);

	if((f->fp = fopen(f->tmpname, "w")) == NULL) {
		error_log("spool_fopen: Unable to open %s for writing: %s",
			f->tmpname, sys_errlist[errno]);
		free(f->name);
		free(f->tmpname);
		free(f);
		return NULL;
	}
	f->next = open_file;
	open_file = f;
	return f->fp;
}

int
spool_fclose(FILE *fp)
{
	struct OpenFiles **f = &open_file;

	while(*f) {
		if((*f)->fp == fp) {
			struct OpenFiles *tmp = *f;
			*f = tmp->next;

			fclose(fp);
			if(unlink(tmp->name) < 0)
				error_log("spool_fclose: Unable to unlink original %s: %s",
					tmp->name, sys_errlist[errno]);

			if(link(tmp->tmpname, tmp->name) < 0) {
				error_log("spool_fclose: Unable to link the spool file to %s: %s",
					tmp->name, sys_errlist[errno]);
			} else
				unlink(tmp->tmpname);

			free(tmp->name);
			free(tmp->tmpname);
			free(tmp);
			return OK;
		}

		f = &((*f)->next);
	}
	return error_log("spool_fclose: No open file found for supplied FILE*");
}
