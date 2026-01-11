#include <sys/queue.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "tools.h"

struct OpenFile {
	LIST_ENTRY(OpenFile) entries;
	char *tmpname;
	char *name;
	FILE *fp;
};

LIST_HEAD(open_file_list, OpenFile);
struct open_file_list open_files = LIST_HEAD_INITIALIZER(open_files);

static struct OpenFile *spool_find_and_dequeue(FILE *fp);
static void free_open_file(struct OpenFile *f);

FILE *
spool_fopen(char *fn)
{
	static int number = 0;
	struct OpenFile *f = malloc_struct(OpenFile);

	if (f == NULL) {
		log_error("spool_fopen: out of mem");
		goto MallocFailed;
	}
	if ((f->name = strdup(fn)) == NULL) {
		log_error("spool_fopen: out of mem in f->name");
		goto NameDupFailed;
	}
	if (asprintf(&f->tmpname, "%s.%d.%d", fn, getpid(), number++) == -1) {
		log_error("spool_fopen: out of mem in f->tmpname");
		goto TmpNameFailed;
	}
	if((f->fp = fopen(f->tmpname, "w")) == NULL) {
		log_error("spool_fopen: Unable to open %s for writing: %s",
			f->tmpname, sys_errlist[errno]);
		goto FopenFailed;
	}

	LIST_INSERT_HEAD(&open_files, f, entries);

	return f->fp;

FopenFailed:
	free(f->tmpname);
TmpNameFailed:
	free(f->name);
NameDupFailed:
	free(f);
MallocFailed:
	return NULL;
}

int
spool_fclose(FILE *fp)
{
	struct OpenFile *tmp = spool_find_and_dequeue(fp);

	if (tmp == NULL) {
		log_error("spool_fclose: No open file found for supplied FILE*");
		goto NotFound;
	}

	if (fclose(tmp->fp) != 0) {
		log_error("spool_fclose: fclose of %s failed", tmp->name);
		goto CloseFailed;
	}

	if (rename(tmp->tmpname, tmp->name) < 0) {
		log_error("spool_fclose: Unable to rename %s -> %s: %m",
			tmp->tmpname, tmp->name);
		goto RenameFailed;
	}

	free_open_file(tmp);

	return OK;

RenameFailed:
CloseFailed:
	unlink(tmp->tmpname);
	free_open_file(tmp);
NotFound:
	return ERROR;
}

int
spool_abort(FILE *fp)
{
	struct OpenFile *tmp = spool_find_and_dequeue(fp);

	if (tmp == NULL) {
		return log_error("spool_fclose: No open file found for supplied FILE*");
	}

	if(unlink(tmp->tmpname) < 0)
		log_error("spool_fclose: Unable to unlink temp %s: %m",
			tmp->tmpname);

	free_open_file(tmp);

	return OK;
}


static struct OpenFile *
spool_find_and_dequeue(FILE *fp)
{
	struct OpenFile *f;

	LIST_FOREACH(f, &open_files, entries) {
		if(f->fp == fp) {
			LIST_REMOVE(f, entries);
			return f;
		}
	}

	return NULL;
}

static void
free_open_file(struct OpenFile *f)
{
	free(f->name);
	free(f->tmpname);
	free(f);
}
