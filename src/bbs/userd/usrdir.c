#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#if HAVE_REGCOMP
#include <regex.h>
#endif /* HAVE_REGCOMP */

#include "c_cmmn.h"
#include "tools.h"
#include "config.h"
#include "bbslib.h"
#include "user.h"

#ifdef DEBUG
void
display_userdir()
{
	struct UserDirectory *dir = UsrDir;

	FILE *fp = fopen("userd.out", "w");
	if(fp == NULL) {
		log_error("couldn't open userd.out for writing");
		return;
	}

	while(dir != NULL) {
		fprintf(fp, "%4d %s [%d] %s (%d) %s", dir->number, dir->call,
			dir->class, port_alias(port_name(dir->port)),
			dir->connect_cnt, ctime(&dir->lastseen));
		NEXT(dir);
	}
}

void
usrdir_lookup(char *s)
{
	struct UserDirectory *dir = UsrDir;

	while(dir) {
		if(!strcmp(s, dir->call))
			return;
		NEXT(dir);
	}
}
#endif

void
usrdir_allocate(char *call, char *s)
{
	struct UserDirectory *dir = malloc_struct(UserDirectory);
	struct UserDirectory **tdir = &UsrDir;

	strncpy(dir->call, call, LenCALL);
	dir->class = parse_callsign(call);

	dir->number = get_number(&s);
	dir->immune = get_number(&s);
	dir->lastseen = get_number(&s);
	dir->port = get_number(&s);
	dir->connect_cnt = get_number(&s);

	while(*tdir != NULL) {
		if((*tdir)->lastseen < dir->lastseen) {
			dir->next = *tdir;
			break;
		}
		tdir = &((*tdir)->next);
	}

	*tdir = dir;
}

static int
bad_char(char *s)
{
	while(*s)
		if(!isalnum(*s++))
			return TRUE;
	return FALSE;
}

int
usrdir_unique_number(int num)
{
	struct UserDirectory *dir = UsrDir;

	while(dir) {
		if(dir->number == num)
			return FALSE;
		NEXT(dir);
	}
	return TRUE;
}

int
usrdir_kill(char *s)
{
	struct UserDirectory *tmp, *dir = UsrDir;

	if(UsrDir == NULL)
		return OK;

	if(!strcmp(UsrDir->call, s)) {
		tmp = UsrDir;
		UsrDir = tmp->next;
		free(tmp);
		return OK;
	}

	while(dir->next) {
		if(!strcmp(s, dir->next->call)) {
			tmp = dir->next;
			dir->next = tmp->next;
			free(tmp);
			return OK;
		}
		NEXT(dir);
	}
	return OK;
}
	
int
usrdir_build(void)
{
	struct dirent *dp;
	char fn[80];
	DIR *dirp;
	FILE *fp;

	dirp = opendir(Userd_Acc_Path);
	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp))
		if(isalnum(dp->d_name[0])) {
			char buf[80];

			if(bad_char(dp->d_name)) {
				kill_user(dp->d_name);
				continue;
			}
				

			sprintf(fn, "%s/%s", Userd_Acc_Path, dp->d_name);
			if((fp = fopen(fn, "r")) == NULL) {
				log_warning("problem opening %s", dp->d_name);
				continue;
			}

			if(fgets(buf, 80, fp) == 0) {
				kill_user(dp->d_name);
				fclose(fp);
				continue;
			}

			fclose(fp);

			if(buf[0] != '+') {
				kill_user(dp->d_name);
				continue;
			}

			usrdir_allocate(dp->d_name, &buf[1]);
		}

	closedir(dirp);
	return OK;
}

void
usrdir_immune(char *call, int immune)
{
	struct UserDirectory *dir = UsrDir;
	
	while(dir) {
		if(!strcmp(call, dir->call)) {
			dir->immune = immune;
			break;
		}
		NEXT(dir);
	}
}

void
usrdir_number(char *call, int number)
{
	struct UserDirectory *dir = UsrDir;
	
	while(dir) {
		if(!strcmp(call, dir->call)) {
			dir->number = number;
			break;
		}
		NEXT(dir);
	}
}

void
usrdir_touch(char *call, int port, long now)
{
	struct UserDirectory *last = NULL, *dir = UsrDir;
	
	while(dir) {
		if(!strcmp(call, dir->call)) {
			dir->connect_cnt++;
			dir->port = port;
			dir->lastseen = now;
			if(last != NULL) {
				last->next = dir->next;
				dir->next = UsrDir;
				UsrDir = dir;
			}
			return;
		}
		last = dir;
		NEXT(dir);
	}
}

char *
find_user(char *s)
{
	struct UserDirectory *dir = UsrDir;
	long number;

	if(*s == 0)
		return "NO, expected a user number\n";

	number = get_number(&s);
	
	while(dir) {
		if(dir->number == number) {
			sprintf(output, "%s\n", dir->call);
			return output;
		}
		NEXT(dir);
	}

	return "NO, user by that number\n";
}

char *
find_user_by_suffix(char *s)
{
	struct UserDirectory *dir = UsrDir;
	char pattern[80];
	int found = FALSE;
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */

	if(*s == 0)
		return "NO, expected a suffix\n";

	sprintf(pattern, "%s$", s);
#if HAVE_REGCOMP
	if (regcomp(&preg, pattern, 0) != 0)
#else
	if(re_comp(pattern))
#endif /* HAVE_REGCOMP */
		return "NO, bad suffix\n";

	output[0] = 0;
	while(dir) {
#if HAVE_REGCOMP
		if (regexec(&preg, dir->call, 0, NULL, 0) == 0) {
#else
		if(re_exec(dir->call) == 1) {
#endif /* HAVE_REGCOMP */
			sprintf(output, "%s %s", output, dir->call);
			found = TRUE;
		}
		NEXT(dir);
	}

	if(!found)
		return "NO, user not found\n";

	strcat(output, "\n");
#if HAVE_REGCOMP
	regfree(&preg);
#endif /* HAVE_REGCOMP */
	return output;
}

