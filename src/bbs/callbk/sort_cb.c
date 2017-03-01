#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "config.h"
#include "bbslib.h"
#include "tools.h"

static void display_indx(FILE *fp);
static int compare(const void *x, const void *y);
static void display_cb(FILE *fp);

static int
compare(const void *x, const void *y)
{
	const struct callbook_index *a = x, *b = y;

	return strcmp(a->key, b->key);
}

int
main(int argc, char *argv[])
{
	struct stat statbuf;
	struct callbook_index *cb;
	char callbook[80];
	char fn[80];
	int c, nel;
	FILE *fp, *tfp;

	if(argc < 2) {
		printf("Enter callbook index file to sort: ");
		safegets(callbook, sizeof(callbook));
	} else
		strcpy(callbook, argv[1]);

	if(stat(callbook, &statbuf) == ERROR) {
		printf("\n** Could not stat callbook index file: %s\n", callbook);
		return 1;
	}

	nel = statbuf.st_size/sizeof(struct callbook_index);
	printf("index file size = %d [%"PROFFTd"/%zd]\n", nel,
		statbuf.st_size, sizeof(struct callbook_index));

	cb = (struct callbook_index *)malloc(statbuf.st_size);

	if((fp = fopen(callbook, "r+")) == NULL) {
		printf("\n** Could not open callbook index file: %s\n", callbook);
		return 1;
	}

	fread(cb, statbuf.st_size, 1, fp);
	qsort((char*)cb, nel, sizeof(struct callbook_index), compare);
	rewind(fp);
	fwrite(cb, statbuf.st_size, 1, fp);
	fclose(fp);
	return 0;
}

static void
display_indx(FILE *fp)
{
	char buf[256];
	struct callbook_index cb;
	int cnt = 0;
	FILE *fpw;

	while(fread(&cb, sizeof(cb), 1, fp)) {
		uint32_t off =
			(cb.loc_xdr[0] << 24) +
			(cb.loc_xdr[0] << 16) +
			(cb.loc_xdr[0] <<  8) +
			(cb.loc_xdr[0]      );

		printf("%6d: %s\t%c%c:%"PRIu32"\n", cnt++, cb.key, cb.area, cb.suffix, off);
	}
}

static void
display_cb(FILE *fp)
{
	char buf[256];
	struct callbook_entry cb;
	int cnt = 0;
	FILE *fpw;

	while(fread(&cb, sizeof(cb), 1, fp)) {
		if(cb.fname[0] == '>')
			printf("%6d: %s%s%s\t%s\n", cnt++,
				cb.prefix, cb.callarea, cb.suffix, cb.fname);
		else
			printf("%6d: %s%s%s\t%s %s\t%s, %s, %s %s\n", cnt++,
				cb.prefix, cb.callarea, cb.suffix, cb.fname, cb.lname,
				cb.addr, cb.city, cb.state, cb.zip);
	}
}
