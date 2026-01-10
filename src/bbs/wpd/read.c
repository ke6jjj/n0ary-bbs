#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"
#include "wp.h"

static int parse_preparsed_entry(struct wp_user_entry *wp, int version,
	char **p, time_t now);
static int parse_freehand_entry(struct wp_user_entry *wp, char **p);

int
iscall(char *s)
{
	int len = strlen(s);
	int alpha = 0;
	int digit = 0;
	int usa = FALSE;

	if(len > 6 || len < 3)
		return FALSE;
	if(*s == 'A' || *s == 'N' || *s == 'K' || *s == 'W') {
		if(len < 4)
			return FALSE;
		usa = TRUE;
	}

	while(*s) {
		if(isalpha(*s))
			alpha++;
		else
			if(isdigit(*s))
				digit++;
			else
				return FALSE;
		s++;
	}

	if(usa && digit != 1)
		return FALSE;

	if(alpha < 2 || digit == 0)
		return FALSE;
	return TRUE;
}

int
read_user_file(char *filename, int depth)
{
	char s[256];
	int cnt = 0;
	FILE *fp = fopen(filename, "r");
	int version = ERROR;
	time_t now = Time(NULL);
	time_t t0;
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(fp == NULL) {
		if (depth == 0) {
			/*
			 * Couldn't open file. Perhaps it needs to
			 * be created. Attempt to do so.
			 */
			write_user_file();

			/*
			 * Now try again (but keep track of the fact that this
			 * is the second time.
			 */
			return read_user_file(filename, depth + 1);
		} else {
			/*
			 * Can't read the file and we've already tried to
			 * create it. Error.
			 */
			return ERROR;
		}
	}

	if(fgets(s, sizeof(s), fp) == 0) {
		fclose(fp);
		return ERROR;
	}

	if(s[2] == 'v')
		version = atoi(&s[3]);

	if(version == ERROR) {
		char *p = s;
		while(fgets(s, sizeof(s), fp)) {
			if(*p == '+')
				break;
		}
		p++;
		get_string(&p);		/*call*/
		get_number(&p);		/*level*/
		get_number(&p);		/*first seen*/
		get_number(&p);		/*last seen*/
		get_number(&p);		/*changed*/

		/* at this point we will either have a number or a homebbs */
		char *call = get_string(&p);
		if(call != NULL && iscall(call)) {
			if(dbug_level & dbgVERBOSE)
				printf("Old database detected\n");
			version = 0;
		} else {
			if(dbug_level & dbgVERBOSE)
				printf("New database with no version number detected\n");
			version = 1;
		}
		rewind(fp);
	}

	if (version > 1) {
		fprintf(stderr,
			"User database version %d is newer than I am!\n",
			version
		);
		fclose(fp);
		return ERROR;
	}

	while(fgets(s, sizeof(s), fp)) {
		char buf[80];
		char *p = s;
		struct wp_user_entry wp;
		int preparsed, ok;

		if(*s == '#' || *s == '\n')
			continue;

		s[strlen(s) - 1] = 0;

		switch(*p) {
		case '+':
			p++;
			preparsed = TRUE;
			break;
		default:
			preparsed = FALSE;
			break;
		}

		if (safe_get_string(buf, &p, sizeof(buf)) != 0)
			continue;

		if(!preparsed && !iscall(buf))
			continue;

		strlcpy(wp.call, buf, sizeof(wp.call));

		if(preparsed) {
			ok = parse_preparsed_entry(&wp, version, &p, now);
		} else {
			ok = parse_freehand_entry(&wp, &p);
		}

		if (ok != 0)
			continue;

		if (hash_insert_or_update_user(&wp) != 0)
			continue;

		cnt++;
	}

	if(dbug_level & dbgVERBOSE)
		printf("Loaded %d users in %"PRTMd" seconds\n", cnt, time(NULL) - t0);
	fclose(fp);
	return OK;
}

static int
parse_preparsed_entry(struct wp_user_entry *wp, int version, char **p,
	time_t now)
{
	char qth[80];

	wp->level = get_number(p);

	wp->firstseen = get_time_t(p);
	wp->seen = get_time_t(p);
	wp->changed = get_time_t(p);

	if(version == 1) {
		wp->last_update_sent = get_time_t(p);
	} else
		wp->last_update_sent = 0;

	if(wp->firstseen > now)
		wp->firstseen = now - (3 * tMonth);
	if(wp->seen > (unsigned)now)
		wp->seen = now - (3 * tMonth);
	if(wp->changed > (unsigned)now)
		wp->changed = now - (3 * tMonth);
	if(wp->last_update_sent > (unsigned)now)
		wp->last_update_sent = now - (3 * tMonth);

	if (safe_get_string(wp->home, p, sizeof(wp->home)) != 0)
		return -1;
	if (safe_get_string(wp->fname, p, sizeof(wp->fname)) != 0)
		return -1;
	if (safe_get_string(wp->zip, p, sizeof(wp->zip)) != 0)
		return -1;
	qth[0] = 0;
	if(**p == 0)
		strlcat(qth, "?", sizeof(qth));
	else {
		while(TRUE) {
			if (safe_cat_string(qth, p, sizeof(qth)) != 0) {
				return -1;
			}
			if(**p == 0)
				break;
			strlcat(qth, " ", sizeof(qth));
		}
	}
	strlcpy(wp->qth, qth, sizeof(wp->qth));

	return 0;
}

static int
parse_freehand_entry(struct wp_user_entry *wp, char **p)
{
	wp->level = get_number(p);

	if (safe_udate2time(&wp->firstseen, p) != 0)
		return -1;
	if (safe_udate2time(&wp->seen, p) != 0)
		return -1;
	if (safe_udate2time(&wp->changed, p) != 0)
		return -1;
	if (safe_udate2time(&wp->last_update_sent, p) != 0)
		return -1;

	if (safe_get_string(wp->home, p, sizeof(wp->home)) != 0)
		return -1;
	if(wp->home[0] == '?')
		return -1;

	if (safe_get_string(wp->fname, p, sizeof(wp->fname)) != 0)
		return -1;
	if (safe_get_string(wp->zip, p, sizeof(wp->zip)) != 0)
		return -1;
	if (safe_get_string(wp->qth, p, sizeof(wp->qth)) != 0)
		return -1;

	return 0;
}

int
read_new_user_file(char *filename)
{
	hash_user_init();
	return read_user_file(filename, 0);
}

int
read_bbs_file(char *filename)
{
	char s[256];
	int cnt = 0;
	FILE *fp = fopen(filename, "r");
	time_t t0;
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(fp == NULL)
		return ERROR;

	if(fgets(s, sizeof(s), fp) == 0) {
		fclose(fp);
		return ERROR;
	}

	while(fgets(s, sizeof(s), fp)) {
		char buf[80];
		char *p = s;
		struct wp_bbs_entry *wp;
		int preparsed = FALSE;

		if(*s == '#')
			continue;

		s[strlen(s)-1] = 0;

		if(*p == '+') {
			p++;
			preparsed = TRUE;
		}

		strlcpy(buf, get_string(&p), sizeof(buf));
		if(!preparsed)
			if(iscall(buf) == FALSE)
				continue;

		if((wp = hash_get_bbs(buf)) == NULL)
			wp = hash_create_bbs(buf);

		strlcpy(wp->call, buf, sizeof(wp->call));
		wp->level = get_number(&p);
		strlcpy(wp->hloc, p, sizeof(wp->hloc));
		cnt++;
	}

	if(dbug_level & dbgVERBOSE)
		printf("Loaded %d bbss in %"PRTMd" seconds\n", cnt, time(NULL) - t0);
	fclose(fp);
	return OK;
}

int
read_new_bbs_file(char *filename)
{
	hash_bbs_init();
	return read_bbs_file(filename);
}

void
startup(void)
{
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("Startup users");
	if(read_new_user_file(Wpd_User_File) == ERROR) {
		printf("Error opening USERFILE %s\n", Wpd_User_File);
		exit(1);
	}

	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("Startup bbss");
	if(read_new_bbs_file(Wpd_Bbs_File) == ERROR) {
		printf("Error opening %s\n", Wpd_Bbs_File);
		exit(1);
	}
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");
}
