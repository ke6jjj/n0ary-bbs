#include <stdio.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"


#define HASH_SIZE	0x10000
#define HASH_MASK	0x0FFFF
#define HASH_SHFT	4

struct bid_entry *bid[HASH_SIZE];

static int hash_in_use = FALSE;

int hash_bucket_size[HASH_SIZE];
int bid_image;

void
hash_init(void)
{
	int i;

	if(hash_in_use) {
		for(i=0; i<HASH_SIZE; i++)
			if(bid[i] != NULL) {
				struct bid_entry *tmp, *b = bid[i];
				while(b) {
					tmp = b;
					NEXT(b);
					free(tmp);
				}
				bid[i] = NULL;
			}
	} else {
		for(i=0; i<HASH_SIZE; i++)
			bid[i] = NULL;
	}

	for(i=0; i<HASH_SIZE; i++)
		hash_bucket_size[i] = 0;

	hash_in_use = TRUE;
	bid_image = CLEAN;
}

static unsigned
hash_key(char *s)
{
	unsigned key = 0;
	int cnt = 0;

	while(*s) {
		key += (key << HASH_SHFT) + *s;
		s++;
	}
	if(key == 0)
		key = HASH_MASK - cnt;
	else
		key %= HASH_MASK;
	return key;
}

struct bid_entry *
hash_get_bid(char *s)
{
	unsigned key = hash_key(s);
	struct bid_entry *b = bid[key];

	while(b != NULL) {
		if(!strcmp(b->str, s))
			break;
		NEXT(b);
	}

	return b;
}

struct bid_entry *
hash_create_bid(char *s)
{
	unsigned key = hash_key(s);
	struct bid_entry *b = malloc_struct(bid_entry);
	
	if(b == NULL) {
		printf("memory allocation failure\n");
		exit(1);
	}
	b->next = bid[key];
	bid[key] = b;

	hash_bucket_size[key]++;
	return b;
}

int
hash_delete_bid(char *s)
{
	unsigned key = hash_key(s);
	struct bid_entry *tmp = NULL, *b = bid[key];

	while(b != NULL) {
		if(!strcmp(b->str, s)) {
			if(tmp)
				tmp->next = b->next;
			else
				bid[key] = b->next;
			free(b);
			return OK;
		}
		tmp = b;
		NEXT(b);
	}

	return ERROR;
}

char *
hash_write(FILE *fp)
{
	int i;

	for(i=0; i<HASH_SIZE; i++) {
		struct bid_entry *b = bid[i];
		while(b) {
			fprintf(fp, "+%s %"PRTMd"\n", b->str, b->seen);
			NEXT(b);
		}
	}
	return "OK\n";
}

char *
hash_cnt(void)
{
	static char buf[4096];
	int i, cnt = 0;
	int max_size[11];
	int max_bucket[11];

	for(i=0; i<10; i++)
		max_size[i] = max_bucket[i] = 0;

	for(i=0; i<HASH_SIZE; i++) {
		int j;
		struct bid_entry *b = bid[i];
		while(b) {
			cnt++;
			NEXT(b);
		}
		for(j=0; j<10; j++) {
			if(hash_bucket_size[i] > max_size[j]) {
				int k;
				for(k=9; k>=j; k--) {
					max_size[k+1] = max_size[k];
					max_bucket[k+1] = max_bucket[k];
				}
				max_size[j] = hash_bucket_size[i];
				max_bucket[j] = i;
				break;
			}
		}
	}

	sprintf(buf, "bid count = %d, image is %s\n", cnt,
		(bid_image == CLEAN) ? "CLEAN" : "DIRTY");
	for(i=0; i<10; i++)
		sprintf(buf, "%smax bucket = %d, size = %d\n",
			buf, max_bucket[i], max_size[i]);
	return buf;
}

void
age(void)
{
	int i, cnt = 0;
	time_t target = Time(NULL) - Bidd_Age;
	time_t t0;
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("Aging bids");
	for(i=0; i<HASH_SIZE; i++) {
		struct bid_entry *t = NULL, *b = bid[i];
		while(b) {
			if(b->seen < target) {
				struct bid_entry *f = b;
				if(t)
					t->next = b->next;
				else
					bid[i] = b->next;
				cnt++;
				NEXT(b);
				free(f);
				continue;
			}
			t = b;
			NEXT(b);
		}
	}
	if(cnt)
		bid_image = DIRTY;
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	if(dbug_level & dbgVERBOSE) {
		printf("aging results = %d\n", cnt);
		printf("         time = %"PRTMd" seconds\n", time(NULL) - t0);
	}
}

