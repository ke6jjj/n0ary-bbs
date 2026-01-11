#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "msgd.h"

char InServer = 'X';

struct route_aliases {
	struct route_aliases *next;
	char name[20];
	char group[1024];
} *RouteAliases = NULL;

struct FwdDir {
	struct FwdDir *next;
	char type;
	int number;
	time_t ctime;
	char alias[20];
} *FwdDir = NULL;

struct Systems {
	struct Systems *next;
	char alias[20];
	char name[20];
} *System = NULL;

struct FwdPool {
	struct FwdPool *next;
	char alias[20];
	int cnt[3];
	int age[7];
} *FwdPool = NULL;

int age_threshold[6] = {
	tHour,
	4*tHour,
	8*tHour,
	tDay,
	2*tDay,
	3*tDay };

static void
read_systems_file()
{
	FILE *fp = fopen(Msgd_System_File, "r");
	struct Systems **sys = &System;
	char buf[256];

	if(fp == NULL)
		return;

	while(fgets(buf, sizeof(buf), fp)) {
		char *p = buf, bbs[20];
		if(*p == ';' ||  *p == '\n' || isspace(*p))
			continue;
		
		strlcpy(bbs, get_string(&p), sizeof(bbs));

		while(*p) {
			(*sys) = malloc_struct(Systems);
			strlcpy((*sys)->alias, get_string(&p),
				sizeof((*sys)->alias));
			strlcpy((*sys)->name, bbs, sizeof((*sys)->name));
			sys = &((*sys)->next);
		}
	}
	fclose(fp);
}
void
free_systems(void)
{
	while(System) {
		struct Systems *sys = System;
		NEXT(System);
		free(sys);
	}
}

void
fwddir_close(void)
{
	struct FwdDir *fwddir;

	while(FwdDir) {
		fwddir = FwdDir;
		NEXT(FwdDir);
		free(fwddir);
	}
	FwdDir = NULL;
}


void
fwddir_open(void)
{
	struct dirent *dp;
	struct FwdDir **fwddir = &FwdDir;
	struct stat sbuf;
	DIR *dirp = opendir(Msgd_Fwd_Dir);

	fwddir_close();

	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
		char type;
		char fn[PATH_MAX];
		int number;
		char *p;

		p = dp->d_name;
		switch(*p) {
		case 'P':
		case 'B':
		case 'T':
			type = *p++;
			break;
		default:
			continue;
		}

		if(*p++ != '.')
			continue;

		number = get_number(&p);
		if(*p++ != '.')
			continue;

		*fwddir = malloc_struct(FwdDir);
		strlcpy((*fwddir)->alias, p, sizeof((*fwddir)->alias));
		(*fwddir)->type = type;
		(*fwddir)->number = number;

		snprintf(fn, sizeof(fn), "%s/%s", Msgd_Fwd_Dir, dp->d_name);
		if(stat(fn, &sbuf) == 0)
			(*fwddir)->ctime = sbuf.st_mtime;	

		fwddir = &((*fwddir)->next);
	}
	closedir(dirp);
}

int
fwddir_check(int number)
{
	struct FwdDir *fwddir;
	int cnt = 0;

	fwddir = FwdDir;

	while(fwddir) {
		if(fwddir->number == number)
			cnt++;
		NEXT(fwddir);
	}
	return cnt;
}

int
fwddir_rename(int orig, int new)
{
	struct FwdDir *fwddir = FwdDir;

	while(fwddir) {
		if(fwddir->number == orig) {
			char ofn[PATH_MAX], nfn[PATH_MAX];
			snprintf(ofn, sizeof(ofn), "%s/%c.%05d.%s",
				Msgd_Fwd_Dir, fwddir->type, orig,
				fwddir->alias);
			snprintf(nfn, sizeof(nfn), "%s/%c.%05d.%s",
				Msgd_Fwd_Dir, fwddir->type, new,
				fwddir->alias);
			if(rename(ofn, nfn) < 0)
				return ERROR;
		}
		NEXT(fwddir);
	}
	return OK;
}

static int
fwddir_remove_stamp(char type, int number, char *alias)
{
	char fn[PATH_MAX];
	snprintf(fn, sizeof(fn), "%s/%c.%05d.%s", Msgd_Fwd_Dir, type, number,
		alias);
	unlink(fn);
	return OK;
}

void
fwddir_kill_stamp(char *fn)
{
	unlink(fn);
}

int
fwddir_kill(int number, char *alias)
{
	struct FwdDir *fwddir;
	int cnt = 0;

	if(alias)
		uppercase(alias);
	fwddir_open();
	fwddir = FwdDir;

	while(fwddir) {
		if(fwddir->number == number) {
			if(alias != NULL) {
				if(!strcmp(alias, fwddir->alias))
					fwddir_remove_stamp(fwddir->type, number, alias);
				else
					cnt++;
			} else 
				fwddir_remove_stamp(fwddir->type, number, fwddir->alias);
		}
		NEXT(fwddir);
	}

	if(cnt == 0) {
		struct msg_dir_entry *m = get_message(number);
		if(m)
			ClrMsgPending(m);
	}
	return OK;
}

int
fwddir_touch(int type, int number, char *alias)
{
	char c, fn[PATH_MAX];
	FILE *fp;

	switch(type & MsgTypeMask) {
	case MsgPersonal:
		c = 'P'; break;
	case MsgBulletin:
		c = 'B'; break;
	case MsgNTS:
		c = 'T'; break;
	default:
		return ERROR;
	}

	snprintf(fn, sizeof(fn), "%s/%c.%05d.%s", Msgd_Fwd_Dir, c, number,
		alias);
	if((fp = fopen(fn, "w")) == NULL) {
		if(dbug_level & dbgVERBOSE)
			printf("Couldn't touch fwd stamp file\n");
		return ERROR;
	}
	fclose(fp);
	return OK;
}

static void
read_route_aliases(FILE *fp)
{
	char *p, buf[80];
	while(fgets(buf, sizeof(buf), fp)) {
		if(buf[0] == '_') {
			struct route_aliases *ra = malloc_struct(route_aliases);
			if(RouteAliases != NULL)
				ra->next = RouteAliases;
			RouteAliases = ra;
			buf[strlen(buf)-1] = 0;

			p = (char*)index(buf, ';');
			if(p) *p = 0;

			p = buf;
			strlcpy(ra->name, get_string(&p), sizeof(ra->name));
			strlcpy(ra->group, p, sizeof(ra->group));
		}
	}
}

static void
free_route_aliases()
{
	struct route_aliases *ra;
	while(RouteAliases) {
		ra = RouteAliases;
		NEXT(RouteAliases);
		free(ra);
	}
}

static void
fwd_check_name(struct active_processes *ap, struct msg_dir_entry *m,
	char *bbs, struct text_line *tl, int check)
{
	struct route_aliases *ra = RouteAliases;

	while(ra) {
		if(!strcmp(ra->name, bbs)) {
			char *p = ra->group;

			while(*p) {
				char q[80];
				strlcpy(q, get_string(&p), sizeof(q));
				fwd_check_name(ap, m, q, tl, check);
			}
			return;
		}
		NEXT(ra);
	}

	if(check == TRUE) {
		snprintf(output, sizeof(output), "%s\n", bbs);
		log_debug("F:%s", output);
		socket_raw_write(ap->fd, output);
	} else {
		while(tl) {
			struct Systems *sys = System;
			if(!strcmp(bbs, tl->s))
				return;
			while(sys) {
				if(!strcmp(bbs, sys->alias)) {
					if(!strcmp(tl->s, sys->name))
						return;
				}
				NEXT(sys);
			}
			NEXT(tl);
		}
		if(!strcmp(bbs, Bbs_Call))
			return;

		m->fwd_cnt++;
		fwddir_touch(m->flags, m->number, bbs);
	}
}

struct text_line *
read_message_path(struct msg_dir_entry *m)
{
	struct text_line *list = NULL;
	char buf[1024];
	FILE *fp;

	snprintf(buf, sizeof(buf), "%s/%05ld", Msgd_Body_Path, m->number);
	if((fp = fopen(buf, "r")) == NULL)
		return NULL;

	while(fgets(buf, sizeof(buf), fp)) {
		char *s;
		if(buf[0] != 'R')
			break;
		
		if((s = (char*)strchr(buf, '@')) == NULL)
			break;

		s++;
		if(*s == ':')
			s++;
		
		textline_append(&list, get_call(&s));
	}
	fclose(fp);
	return list;
}

static void
forward_message_to(struct active_processes *ap,
	struct msg_dir_entry *m, char *list, int check)
{
	struct text_line *tl = read_message_path(m);

	m->fwd_cnt = 0;
	while(*list) {
		char bbs[80];
		strlcpy(bbs, get_string(&list), sizeof(bbs));
		fwd_check_name(ap, m, bbs, tl, check);
	}
	textline_free(tl);
}

/* new Route file structure...

	The route file now looks like the Translave file. It is composed of
	a RULES:OPERATION. The Route file will be scanned a single time
	per message, first match is accepted. Each rule will be tested
	fully before going on to the next rule.

	>USA:	SBAY		; if to USA then forward to SBAY
	@USA:	NBAY		; if @USA then forward to NBAY
	USA:	GATEWAY		; if USA appears in HLOC forward to GATEWAY

	This allows for such things as this:

	>SALE @ALLUS:	SBAY
	@ALLUS:			EVERYONE

	Do to the order all SALE@ALLUS messages will only go to SBAY. All
	other ALLUS messages will go to EVERYONE.

	NOTE: This bbs must appear in the Route file as well so the following
	can occur:

	>WB6RIG @N0ARY:	WB6RIG		;personal tnc forwarding
	@N0ARY:			N0ARY
*/

int
set_forwarding(struct active_processes *ap, struct msg_dir_entry *m, int check)
{
	FILE *fp = fopen(Msgd_Route_File, "r");
	char buf[256];
	int found = FALSE;
	time_t now = Time(NULL);

	if(fp == NULL)
		return ERROR;
	if(check == FALSE)
		if(IsMsgKilled(m) || IsMsgNoFwd(m)) {
			fclose(fp);
			return OK;
		}

	read_systems_file();
	read_route_aliases(fp);
	rewind(fp);

	while(fgets(buf, sizeof(buf), fp)) {
		char *p = buf;
		char *list;

		/* skip over comments and aliases */

		if(*p == ';' || *p == '_' || *p == '\n' || isspace(*p))
			continue;

		/* find the separator, this will identify the beginning of
		 * the distribution list. If it isn't there then abort this line.
		 */

		if((list = (char*)strchr(p, cSEPARATOR)) == NULL)
			continue;
		*list++ = 0;
		NextChar(list);
		if(*list == 0)
			/* oops no distribution */
			continue;

		/* now locate a comment if it exists and remove it. */

		if((p = (char*)strchr(list, ';')) != NULL) {
			do {
				*p-- = 0;
			} while(isspace(*p));
		}

			/* buf points to match criteria, list points to who we will
			 * forward to if match is successful.
			 */
		
		if(message_matches_criteria(buf, m, now) == TRUE) {
			if(check == TRUE) {
				snprintf(output, sizeof(output), "FwdOn: %s\n",
					buf);
				log_debug("F:%s", output);
				socket_raw_write(ap->fd, output);
			}
			forward_message_to(ap, m, list, check);
			found = TRUE;
			break;
		}
	}

	if(!check) {
		if(!found) {
			if(m->to.at.str[0] && (m->flags & MsgActive)) {
				rfc822_append(m->number, rHELDBY, "BBS");
				rfc822_append(m->number, rHELDWHY, "No route present for HLOC");
				SetMsgHeld(m);
			}
		} else
			if(m->fwd_cnt)
				SetMsgPending(m);
	}

	fclose(fp);
	free_route_aliases();
	free_systems();
	return OK;
}

void
pending_fwd_num(struct active_processes *ap, int num)
{
	struct FwdDir *fwddir;

	fwddir_open();
	fwddir = FwdDir;

	while(fwddir) {
		char buf[80];
		if(fwddir->number == num) {
			snprintf(buf, sizeof(buf), "%s\n", fwddir->alias);
			log_debug("F:%s", buf);
			socket_raw_write(ap->fd, buf);
		}
		NEXT(fwddir);
	}
}

void
pending_fwd(struct active_processes *ap, char *call, char msgtype)
{
	struct FwdDir *fwddir;

	if(call)
		uppercase(call);
	ToUpper(msgtype);

	fwddir_open();
	fwddir = FwdDir;

	while(fwddir) {
		char buf[80];
		if(call == NULL) {
			snprintf(buf, sizeof(buf), "%c.%05d.%s\n",
				fwddir->type, fwddir->number, fwddir->alias);
			log_debug("F:%s", buf);
			socket_raw_write(ap->fd, buf);
		} else
			if(!strcmp(call, fwddir->alias)) {
				if(msgtype == 0 || msgtype == fwddir->type) {
					snprintf(buf, sizeof(buf), "%05d\n",
						fwddir->number);
					log_debug("F:%s", buf);
					socket_raw_write(ap->fd, buf);
				}
			}
		NEXT(fwddir);
	}
}

void
fwd_stats(void)
{
	struct FwdDir *fwddir;
	struct FwdPool *fp;
	time_t delta, now = Time(NULL);
	int i;

	fwddir_open();
	fwddir = FwdDir;

	while(fwddir) {
		fp = FwdPool;
		if(fp != NULL) {
			while(fp) {
				if(!strcmp(fp->alias, fwddir->alias))
					break;
				NEXT(fp);
			}
		}
		if(fp == NULL) {
			fp = malloc_struct(FwdPool);
			fp->next = FwdPool;
			FwdPool = fp;
			strlcpy(fp->alias, fwddir->alias, sizeof(fp->alias));
		}

		switch(fwddir->type) {
		case 'P':
			fp->cnt[0]++;
			break;
		case 'B':
			fp->cnt[1]++;
			break;
		default:
			fp->cnt[2]++;
		}

		delta = now - fwddir->ctime;

		for(i=0; i<6; i++) 
			if(delta < age_threshold[i])
				break;
		fp->age[i]++;
					
		NEXT(fwddir);
	}

/*
			p/b/t      total   <1h   <4h   <8h   <1d   <2d   <3d   >3d
	N6QMY	20/50/0     300    250   100   100   100   100   100   100
*/

	fp = FwdPool;
	snprintf(output, sizeof(output),
		"%s\nCall\tp/b/t\ttotal\t<1h\t<4h\t<8h\t<1d\t<2d\t<3d\t>3d\n", output);

	while(fp) {
		struct FwdPool *tmp = fp;
		snprintf(output, sizeof(output),
			"%s%s\t%d/%d/%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
			output, fp->alias, fp->cnt[0], fp->cnt[1], fp->cnt[2],
			fp->cnt[0]+fp->cnt[1]+fp->cnt[2],
			fp->age[0], fp->age[1], fp->age[2], fp->age[3], fp->age[4],
			fp->age[5], fp->age[6]);
		NEXT(fp);
		free(tmp);
	}
	FwdPool = NULL;
}

