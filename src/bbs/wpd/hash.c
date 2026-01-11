#include <stdio.h>
#include <stdlib.h>
#if HAVE_REGCOMP
#include <regex.h>
#endif
#include <unistd.h>

#include "c_cmmn.h"
#include "tools.h"
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

static int hash_write_user(FILE *fp);
static int hash_write_bbs(FILE *fp);
static int dump_user_entry(FILE *fp, struct wp_user_entry *wp);

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
new_user_entry(void)
{
	struct wp_user_entry *wp = malloc_struct(wp_user_entry);
	if (wp == NULL) {
		log_error("memory allocation failure");
		exit(1);
	}

	return wp;
}

int
hash_insert_user(char *call, struct wp_user_entry *wp)
{
	unsigned key = hash_user_key(call);
	if (user[key] != NULL && strcmp(user[key]->call, call) == 0) {
		/* Duplicate entry! */
		return -1;
	}

	wp->next = user[key];
	user[key] = wp;

	return 0;
}

int
hash_insert_or_update_user(struct wp_user_entry *src)
{
	int existing = 1;
	struct wp_user_entry *wp, *next;

	if ((wp = hash_get_user(src->call)) == NULL) {
		existing = 0;
		wp = new_user_entry();
	} else {
		next = wp->next;
	}

	memcpy(wp, src, sizeof(*src));
	wp->next = next;

	if (!existing) {
		if (hash_insert_user(wp->call, wp) != 0) {
			free(wp);
			return -1;
		}
	}

	return 0;
}

struct wp_bbs_entry *
hash_create_bbs(char *s)
{
	unsigned key = hash_bbs_key(s);
	struct wp_bbs_entry *wp = malloc_struct(wp_bbs_entry);
	
	if(wp == NULL) {
		log_error("memory allocation failure");
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

int
hash_write(FILE *fp, int mode)
{
	int i;

	if(mode == WriteALL || mode == WriteUSER) {
		return hash_write_user(fp);
	} else {
		return hash_write_bbs(fp);
	}
}

static int
hash_write_user(FILE *fp)
{
	size_t i;
	int res;

	for(i=0; i<USER_HASH_SIZE; i++) {
		struct wp_user_entry *wp = user[i];
		while(wp) {
			res = dump_user_entry(fp, wp);
			if (res < 0)
				return res;
			NEXT(wp);
		}
	}

	return 0;
}

static int
hash_write_bbs(FILE *fp)
{
	size_t i;
	int res;

	for(i=0; i<BBS_HASH_SIZE; i++) {
		struct wp_bbs_entry *wp = bbs[i];
		while(wp) {
			res = fprintf(fp,
				"+%s %ld %s\n",
				wp->call, wp->level, wp->hloc);
			if (res < 0)
				return res;
			NEXT(wp);
		}
	}

	return 0;
}

static int
dump_user_entry(FILE *fp, struct wp_user_entry *wp)
{
	char buf[1024];
	int res;

	res = snprintf(buf, sizeof(buf),
		"+%s "
		"%ld "
		"%"PRTMd" %"PRTMd" %"PRTMd" %"PRTMd" "
		"%s %s %s %s\n",
		wp->call,
		wp->level,
		wp->firstseen,
		wp->seen,
		wp->changed,
		wp->last_update_sent,
		wp->home,
		wp->fname,
		wp->zip,
		wp->qth
	);
	if (res < 0)
		return res;

	if(dbug_level & dbgVERBOSE) {
		fputs(buf, stdout);
		fflush(stdout);
	}

	return fputs(buf, fp);
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

	log_debug("current time = %"PRTMd, t);
	log_debug("refresh threshold = %"PRTMd, refresh);

	*gcnt = *lcnt = 0;

	for(i=0; i<USER_HASH_SIZE; i++) {
		struct wp_user_entry *wp = user[i];
		while(wp) {
			int update = FALSE;

			if(wp->level < WP_Init && strcmp(wp->home, Bbs_Call) == 0) {
				if(wp->changed > wp->last_update_sent) {
					log_debug("%s changed since update %"PRTMd" > %"PRTMd"",
						wp->call, wp->changed, wp->last_update_sent);
					update = TRUE;
				} else
				if((wp->seen > wp->last_update_sent) &&
					(wp->last_update_sent < refresh)) {
					log_debug("%s seen in last two weeks %"PRTMd" > %"PRTMd,
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
	log_debug("%d %d entries match request", *gcnt, *lcnt);
	return OK;
}

void
hash_search_user(int fd, char *pat, int mode)
{
	char buf[80];
	int i;
	int found = FALSE;
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif

#if HAVE_REGCOMP
	if (regcomp(&preg, pat, 0) != 0) {
#else
	if(re_comp(pat) != NULL) {
#endif /* HAVE_REGCOMP */
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

#if HAVE_REGCOMP
			if (regexec(&preg, p, 0, NULL, 0) == 0) { /* Found */
#else
			if(re_exec(p) == 1) {
#endif /* HAVE_REGCOMP */
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
#if HAVE_REGCOMP
	regfree(&preg);
#endif /* HAVE_REGCOMP */
	return;
}

void
hash_search_bbs(int fd, char *pat)
{
	char buf[80];
	int i;
	int found = FALSE;
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */

#if HAVE_REGCOMP
	if (regcomp(&preg, pat, 0) != 0) {
#else
	if(re_comp(pat) != NULL) {
#endif /* HAVE_REGCOMP */
		char *c = "Bad regular expression\n";
		write(fd, c, strlen(c));
		return;
	}

	for(i=0; i<BBS_HASH_SIZE; i++) {
		struct wp_bbs_entry *wp = bbs[i];
		while(wp) {
#if HAVE_REGCOMP
			if (regexec(&preg, wp->hloc, 0, NULL, 0) == 0) { /* Found */
#else
			if(re_exec(wp->hloc) == 1) {
#endif /* HAVE_REGCOMP */
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
#if HAVE_REGCOMP
	regfree(&preg);
#endif
	return;
}
