#include <stdio.h>
#ifndef SUNOS
#include <regex.h>
#endif

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

#define USER_HASH_SIZE	0x10000
#define USER_HASH_MASK	0x0FFFF
#define USER_HASH_SHFT	3

#define BBS_HASH_SIZE	0x1000
#define BBS_HASH_MASK	0x0FFF
#define BBS_HASH_SHFT	1


struct wp_user_entry *user[USER_HASH_SIZE];
struct wp_bbs_entry *bbs[BBS_HASH_SIZE];

static int user_hash_in_use = FALSE;
static int bbs_hash_in_use = FALSE;

int user_image, bbs_image;

void
hash_user_init(void)
{
	int i;

	if(user_hash_in_use) {
		for(i=0; i<USER_HASH_SIZE; i++)
			if(user[i] != NULL) {
				struct wp_user_entry *tmp, *wp = user[i];
				while(wp) {
					tmp = wp;
					NEXT(wp);
					free(tmp);
				}
				user[i] = NULL;
			}
	} else {
		for(i=0; i<USER_HASH_SIZE; i++)
			user[i] = NULL;
	}

	user_hash_in_use = TRUE;
	user_image = CLEAN;
}

void
hash_bbs_init(void)
{
	int i;

	if(bbs_hash_in_use) {
		for(i=0; i<BBS_HASH_SIZE; i++)
			if(bbs[i] != NULL) {
				struct wp_bbs_entry *tmp, *wp = bbs[i];
				while(wp) {
					tmp = wp;
					NEXT(wp);
					free(tmp);
				}
				bbs[i] = NULL;
			}
	} else {
		for(i=0; i<BBS_HASH_SIZE; i++)
			bbs[i] = NULL;
	}

	bbs_hash_in_use = TRUE;
	bbs_image = CLEAN;
}

static unsigned
hash_bbs_key(char *s)
{
	unsigned key = 0;
	char c = *s++;

	while(*s) {
		if(*s > 0x40)
			key = (key<<BBS_HASH_SHFT) + (*s - 0x41);
		else
			key *= (*s - 0x30);
		s++;
	}

	if(c > 0x40)
		key = (key<<BBS_HASH_SHFT) + (c - 0x41);
	else
		key *= (c - 0x30);
	key %= BBS_HASH_MASK;

	return key;
}

static unsigned
hash_user_key(char *s)
{
	unsigned key = 0;
	char c = *s++;

	while(*s) {
		if(*s > 0x40)
			key = (key<<USER_HASH_SHFT) + (*s - 0x41);
		else
			key *= (*s - 0x30);
		s++;
	}

	if(c > 0x40)
		key = (key<<USER_HASH_SHFT) + (c - 0x41);
	else
		key *= (c - 0x30);
	key %= USER_HASH_MASK;

	return key;
}

struct wp_user_entry *
hash_get_user(char *s)
{
	unsigned key = hash_user_key(s);
	struct wp_user_entry *wp = user[key];

	while(wp != NULL) {
		if(!strcmp(wp->call, s))
			break;
		NEXT(wp);
	}

	return wp;
}

struct wp_bbs_entry *
hash_get_bbs(char *s)
{
	unsigned key = hash_bbs_key(s);
	struct wp_bbs_entry *wp = bbs[key];

	while(wp != NULL) {
		if(!strcmp(wp->call, s))
			break;
		NEXT(wp);
	}

	return wp;
}

struct wp_user_entry *
hash_create_user(char *s)
{
	unsigned key = hash_user_key(s);
	struct wp_user_entry *wp = malloc_struct(wp_user_entry);
	
	if(wp == NULL) {
		printf("memory allocation failure\n");
		exit(1);
	}
	wp->next = user[key];
	user[key] = wp;

	return wp;
}

struct wp_bbs_entry *
hash_create_bbs(char *s)
{
	unsigned key = hash_bbs_key(s);
	struct wp_bbs_entry *wp = malloc_struct(wp_bbs_entry);
	
	if(wp == NULL) {
		printf("memory allocation failure\n");
		exit(1);
	}
	wp->next = bbs[key];
	bbs[key] = wp;

	return wp;
}

int
hash_delete_user(char *s)
{
	unsigned key = hash_user_key(s);
	struct wp_user_entry *tmp = NULL, *wp = user[key];

	while(wp != NULL) {
		if(!strcmp(wp->call, s)) {
			if(tmp)
				tmp->next = wp->next;
			else
				user[key] = wp->next;
			free(wp);
			return OK;
		}
		tmp = wp;
		NEXT(wp);
	}

	return ERROR;
}

int
hash_delete_bbs(char *s)
{
	unsigned key = hash_bbs_key(s);
	struct wp_bbs_entry *tmp = NULL, *wp = bbs[key];

	while(wp != NULL) {
		if(!strcmp(wp->call, s)) {
			if(tmp)
				tmp->next = wp->next;
			else
				bbs[key] = wp->next;
			free(wp);
			return OK;
		}
		tmp = wp;
		NEXT(wp);
	}

	return ERROR;
}

char *
hash_write(FILE *fp, int mode)
{
	int i;

	if(mode == WriteALL || mode == WriteUSER) {
		for(i=0; i<USER_HASH_SIZE; i++) {
			struct wp_user_entry *wp = user[i];
			while(wp) {

				if(dbug_level & dbgVERBOSE) {
					char buf[256];
					sprintf(buf, "+%s %d %u %u %u %u %s %s %s %s\n",
						wp->call, wp->level, (unsigned)wp->firstseen,
						(unsigned)wp->seen, (unsigned)wp->changed,
						(unsigned)wp->last_update_sent,
						wp->home, wp->fname, wp->zip, wp->qth);
					printf("%s", buf);
				}

				fprintf(fp, "+%s %d %u %u %u %u %s %s %s %s\n",
					wp->call, wp->level, (unsigned)wp->firstseen,
					(unsigned)wp->seen, (unsigned)wp->changed,
					(unsigned)wp->last_update_sent,
					wp->home, wp->fname, wp->zip, wp->qth);

				NEXT(wp);
			}
		}
	} else {
		for(i=0; i<BBS_HASH_SIZE; i++) {
			struct wp_bbs_entry *wp = bbs[i];
			while(wp) {
				fprintf(fp, "+%s %d %s\n", wp->call, wp->level, wp->hloc);
				NEXT(wp);
			}
		}
	}
	return "OK\n";
}

char *
hash_bbs_cnt(void)
{
	static char buf[80];
	int i, cnt = 0;

	for(i=0; i<BBS_HASH_SIZE; i++) {
		struct wp_bbs_entry *wp = bbs[i];
		while(wp) {
			cnt++;
			NEXT(wp);
		}
	}

	sprintf(buf, "bbs count = %d, image is %s\n", cnt,
		(bbs_image == CLEAN) ? "CLEAN" : "DIRTY");
	return buf;
}

char *
hash_user_cnt(void)
{
	static char buf[80];
	int i, cnt = 0;

	for(i=0; i<USER_HASH_SIZE; i++) {
		struct wp_user_entry *wp = user[i];
		while(wp) {
			cnt++;
			NEXT(wp);
		}
	}

	sprintf(buf, "user count = %d, image is %s\n", cnt,
		(user_image == CLEAN) ? "CLEAN" : "DIRTY");
	return buf;
}

int
hash_gen_update(FILE *gfp, int *gcnt, FILE *lfp, int *lcnt)
{
	time_t t = Time(NULL);
	int i;
	time_t refresh = t - Wpd_Refresh;
	time_t threshold = t - Wpd_Age;

	if(dbug_level & dbgVERBOSE) {
		printf("current time = %d\n", t);
		printf("refresh threshold = %d\n\n", refresh);
	}

	*gcnt = *lcnt = 0;

	for(i=0; i<USER_HASH_SIZE; i++) {
		struct wp_user_entry *wp = user[i];
		while(wp) {
			int update = FALSE;

			if(wp->level < WP_Init) {
				if(wp->changed > wp->last_update_sent) {
					if(dbug_level & dbgVERBOSE)
						printf("%s changed since update %d > %d\n",
							wp->call, wp->changed, wp->last_update_sent);
					update = TRUE;
				} else
				if((wp->seen > wp->last_update_sent) &&
					(wp->last_update_sent < refresh)) {
					if(dbug_level & dbgVERBOSE)
						printf("%s seen in last two weeks %d > %d\n",
							wp->call, wp->seen, wp->last_update_sent);
					update = TRUE;
				}
			}

			if(update) {
				char hloc[80];
				char buf[256];
				struct wp_bbs_entry *wpb = hash_get_bbs(wp->home);

				if(wpb != NULL) {
					if(wpb->hloc[0] && wpb->hloc[0] != '?')
						sprintf(hloc, ".%s", wpb->hloc);
					else
						hloc[0] = 0;
	
					sprintf(buf, "On %s %s/%c @ %s%s zip %s %s %s\n",
						time2udate((wp->seen>wp->changed) ? wp->seen:wp->changed),
						wp->call,
						(wp->level >= WP_Guess) ? 'G' : 'U',
						wp->home, hloc, wp->zip, wp->fname, wp->qth);
					(*gcnt)++;

					fputs(buf, gfp);
					if(lfp != NULL && wp->level < WP_Guess) {
						fputs(buf, lfp);
						(*lcnt)++;
					}
				}
				wp->last_update_sent = t;
			} else
				if(wp->changed < threshold && wp->seen < threshold) {
					switch(wp->level) {
					case WP_Sysop:
						break;
					case WP_Guess:
					case WP_User:
						wp->level = WP_Init;
						user_image = DIRTY;
						break;
					}
				}

			NEXT(wp);
		}
	}
	if(dbug_level & dbgVERBOSE)
		printf("%d %d entries match request\n", *gcnt, *lcnt);
	return OK;
}

void
hash_search_user(int fd, char *pat, int mode)
{
	char buf[80];
	int i;
	int found = FALSE;

	if(re_comp(pat) != NULL) {
		char *c = "Bad regular expression\n";
		write(fd, c, strlen(c));
		return;
	}

	for(i=0; i<USER_HASH_SIZE; i++) {
		struct wp_user_entry *wp = user[i];
		while(wp) {
			char *p;
			switch(mode) {
			case wFNAME:
				p = wp->fname; break;
			case wQTH:
				p = wp->qth; break;
			case wZIP:
				p = wp->zip; break;
			case wHOME:
				p = wp->home; break;
			}

			if(re_exec(p) == 1) {
				found = TRUE;
				sprintf(buf, "%s @ %s %s %s %s\n", wp->call, wp->home,
					wp->fname, wp->zip, wp->qth);
				write(fd, buf, strlen(buf));
			}
			NEXT(wp);
		}
	}

	if(!found) {
		char *c = "No matches found\n";
		write(fd, c, strlen(c));
	}
	return;
}

void
hash_search_bbs(int fd, char *pat)
{
	char buf[80];
	int i;
	int found = FALSE;

	if(re_comp(pat) != NULL) {
		char *c = "Bad regular expression\n";
		write(fd, c, strlen(c));
		return;
	}

	for(i=0; i<BBS_HASH_SIZE; i++) {
		struct wp_bbs_entry *wp = bbs[i];
		while(wp) {
			if(re_exec(wp->hloc) == 1) {
				found = TRUE;
				sprintf(buf, "%s.%s\n", wp->call, wp->hloc);
				write(fd, buf, strlen(buf));
			}
			NEXT(wp);
		}
	}

	if(!found) {
		char *c = "No matches found\n";
		write(fd, c, strlen(c));
	}
	return;
}
