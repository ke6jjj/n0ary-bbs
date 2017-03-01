#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"

int
read_file(char *filename)
{
	char s[256];
	int cnt = 0;
	FILE *fp = fopen(filename, "r");
	time_t t0;
#if 0
	int version = 1;
#endif
	if(dbug_level)
		t0 = time(NULL);

	if(fp == NULL)
		return OK;

	if(fgets(s, 256, fp) == 0) {
		fclose(fp);
		return ERROR;
	}
#if 0	
	if(s[2] == 'v')
		version = atoi(&s[3]);
#endif

	while(fgets(s, 256, fp)) {
		char buf[80];
		char *p = s;
		struct bid_entry *b;
		int preparsed = FALSE;

		if(*s == '#')
			continue;

		s[strlen(s) - 1] = 0;

		if(*p == '+') {
			p++;
			preparsed = TRUE;
		}

		strcpy(buf, get_string(&p));
		if((b = hash_get_bid(buf)) == NULL)
			b = hash_create_bid(buf);

		strcpy(b->str, buf);

		if(preparsed)
			b->seen = get_number(&p);
		else
			b->seen = date2time(get_string(&p));
		cnt++;
	}

	if(dbug_level)
		printf("Loaded %d bids in %"PRTMd" seconds\n", cnt, time(NULL) - t0);
	fclose(fp);
	return OK;
}

int
read_new_file(char *filename)
{
	hash_init();
	return read_file(filename);
}

