#include <stdio.h>

#include "config.h"
#include "bbslib.h"

main(int argc, char *argv[])
{
	char callbook[80];
	char fn[80];
	char *p;
	FILE *fp, *tfp;

	if(argc == 0) {
		printf("Enter callbook file to display: ");
		gets(callbook);
	} else
		strcpy(callbook, argv[1]);

	if((fp = fopen(callbook, "r")) == NULL) {
		printf("\n** Could not open callbook file: %s\n", callbook);
		exit(1);
	}
	p = (char*)rindex(callbook, '.');

	if(p != NULL && !strcmp(p+1, "indx"))
		display_indx(fp);
	else
		display_cb(fp);
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
