#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

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

    if(fgets(s, 256, fp) == 0) {
		fclose(fp);
		return ERROR;
	}

	if(s[2] == 'v')
		version = atoi(&s[3]);

	if(version == ERROR) {
		char *p = s;
		while(fgets(s, 256, fp)) {
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
		if(iscall(get_string(&p))) {
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

	while(fgets(s, 256, fp)) {
		char buf[80];
		char *p = s;
		struct wp_user_entry *wp;
		int preparsed;

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

		strcpy(buf, get_string(&p));

		if(!preparsed)
			if(iscall(buf) == FALSE)
				continue;

		if((wp = hash_get_user(buf)) == NULL)
			wp = hash_create_user(buf);

		strcpy(wp->call, buf);
		wp->level = get_number(&p);


		if(preparsed) {
			char qth[80];
			wp->firstseen = get_number(&p);
			wp->seen = get_number(&p);
			wp->changed = get_number(&p);

			if(version == 1)
				wp->last_update_sent = get_number(&p);
			else
				wp->last_update_sent = 0;

			if((unsigned)wp->firstseen > (unsigned)now)
				wp->firstseen = now - (3 * tMonth);
			if((unsigned)wp->seen > (unsigned)now)
				wp->seen = now - (3 * tMonth);
			if((unsigned)wp->changed > (unsigned)now)
				wp->changed = now - (3 * tMonth);
			if((unsigned)wp->last_update_sent > (unsigned)now)
				wp->last_update_sent = now - (3 * tMonth);

			strncpy(wp->home, get_string(&p), LenCALL);
			strncpy(wp->fname, get_string(&p), LenFNAME);
			strncpy(wp->zip, get_string(&p), LenZIP);
			qth[0] = 0;
			if(*p == 0)
				strcat(qth, "?");
			else {
				while(TRUE) {
					strcat(qth, get_string(&p));
					if(*p == 0)
						break;
					strcat(qth, " ");
				}
				qth[LenQTH-1] = 0;
			}
			strcpy(wp->qth, qth);
			cnt++;
			continue;
		} else {
			wp->firstseen = date2time(get_string(&p));
			wp->seen = date2time(get_string(&p));
			wp->changed = date2time(get_string(&p));
			wp->last_update_sent = date2time(get_string(&p));

			if(*p) {
				strncpy(buf, get_string(&p), 80);
				buf[LenCALL-1] = 0;
				strcpy(wp->home, buf);
				if(wp->home[0] == '?') {
					hash_delete_user(wp->call);
					continue;
				}
	
				if(*p) {
					strncpy(buf, get_string(&p), 80);
					buf[LenFNAME-1] = 0;
					strcpy(wp->fname, buf);
	
					if(*p) {
						strncpy(buf, get_string(&p), 80);
						buf[LenZIP-1] = 0;
						strcpy(wp->zip, buf);
				
						if(*p) {
							strncpy(buf, p, 80);
							buf[LenQTH-1] = 0;
							strcpy(wp->qth, buf);
							continue;
						}
					}
				}
			}
		}
		hash_delete_user(wp->call);
	}

	if(dbug_level & dbgVERBOSE)
		printf("Loaded %d users in %"PRTMd" seconds\n", cnt, time(NULL) - t0);
	fclose(fp);
	return OK;
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
#if 0
	int version = 1;
#endif
	time_t t0;
	if(dbug_level & dbgVERBOSE)
		t0 = time(NULL);

	if(fp == NULL)
		return ERROR;

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
		struct wp_bbs_entry *wp;
		int preparsed = FALSE;

		if(*s == '#')
			continue;

		s[strlen(s)-1] = 0;

		if(*p == '+') {
			p++;
			preparsed = TRUE;
		}

		strcpy(buf, get_string(&p));
		if(!preparsed)
			if(iscall(buf) == FALSE)
				continue;

		if((wp = hash_get_bbs(buf)) == NULL)
			wp = hash_create_bbs(buf);

		strcpy(wp->call, buf);
		wp->level = get_number(&p);
		strncpy(wp->hloc, p, LenHLOC-1);
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
