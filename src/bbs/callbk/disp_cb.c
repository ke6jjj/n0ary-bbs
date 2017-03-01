#include <stdio.h>
#include <inttypes.h>

#include "config.h"
#include "bbslib.h"
#include "tools.h"

static void display_indx(FILE *fp);
static void display_cb(FILE *fp);

int
main(int argc, char *argv[])
{
	char callbook[80];
	char fn[80];
	char *p;
	FILE *fp, *tfp;

	if(argc < 2) {
		printf("Enter callbook file to display: ");
		safegets(callbook, sizeof(callbook));
	} else
		strcpy(callbook, argv[1]);

	if((fp = fopen(callbook, "r")) == NULL) {
		printf("\n** Could not open callbook file: %s\n", callbook);
		return 1;
	}
	p = (char*)rindex(callbook, '.');

	if(p != NULL && !strcmp(p+1, "indx"))
		display_indx(fp);
	else
		display_cb(fp);
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
			(cb.loc_xdr[1] << 16) +
			(cb.loc_xdr[2] <<  8) +
			(cb.loc_xdr[3]      );

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
