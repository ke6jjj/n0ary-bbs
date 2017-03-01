#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "bbslib.h"

int
compare(struct callbook_index *a, struct callbook_index *b)
{
	return strcmp(a->key, b->key);
}

main(int argc, char *argv[])
{
	struct stat statbuf;
	struct callbook_index *cb;
	char callbook[80];
	char fn[80];
	int c, nel;
	FILE *fp, *tfp;

	if(argc < 1) {
		printf("Enter callbook index file to sort: ");
		gets(callbook);
	} else
		strcpy(callbook, argv[1]);

	if(stat(callbook, &statbuf) == ERROR) {
		printf("\n** Could not stat callbook index file: %s\n", callbook);
		exit(1);
	}

	nel = statbuf.st_size/sizeof(struct callbook_index);
	printf("index file size = %d [%d/%d]\n", nel,
		statbuf.st_size, sizeof(struct callbook_index));

	cb = (struct callbook_index *)malloc(statbuf.st_size);

	if((fp = fopen(callbook, "r+")) == NULL) {
		printf("\n** Could not open callbook index file: %s\n", callbook);
		exit(1);
	}

	fread(cb, statbuf.st_size, 1, fp);
	qsort((char*)cb, nel, sizeof(struct callbook_index), compare);
	rewind(fp);
	fwrite(cb, statbuf.st_size, 1, fp);
	fclose(fp);
	exit(0);
}

display_indx(FILE *fp)
{
	char buf[256];
	struct callbook_index cb;
	int cnt = 0;
	FILE *fpw;

	while(fread(&cb, sizeof(cb), 1, fp))
		printf("%6d: %s\t%c%c:%d\n", cnt++, cb.key, cb.area, cb.suffix, cb.loc);
}

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
