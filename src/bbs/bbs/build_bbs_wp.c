#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dirent.h>

#define LenCALL		7
#define LenHLOC		48
#define LenFNAME    20
#define LenLNAME    20
#define LenQTH      30
#define LenZIP      6
#define LenHOME     7


#include "c_cmmn.h"
#include "config.h"
#include "vars.h"
#include "wp.h"

struct wp_list {
	struct wp_list *next, *last;
	int cnt;
	char call[7];

	int hloc_cnt;
	char num[10][20];
	char hloc[10][48];
	char how[10];
} *wpl = NULL;

struct wp_bbs_entry {
    long    date;
    int     how;
    char    call[LenCALL];
    char    locale[2][6];
    char    addr[3][4];
};
#define SizeofWpBbs sizeof(struct wp_bbs_entry)

struct wp_list *
allocate(struct wp_list *lwp)
{
	struct wp_list *wp = (struct wp_list*)calloc(1, sizeof(struct wp_list));

	if(lwp) {
		if(lwp->last)
			lwp->last->next = wp;
		wp->last = lwp->last;
		lwp->last = wp;
		wp->next = lwp;
	}
	return wp;
}

static void
check_list(char how, char *call, char *hloc, char *num)
{
	struct wp_list *nwp, *wp = wpl;

	if(wp == NULL) {
		wpl = allocate(NULL);
		strcpy(wpl->call, call);
		strcpy(wpl->hloc[0], hloc);
		strcpy(wpl->num[0], num);
		wpl->how[0] = how;
		wpl->cnt++;
		wpl->hloc_cnt++;
		return;
	}

	while(1) {
		int result = strcmp(wp->call, call);

		if(result == 0) {
			int i;

			for(i=0; i<wp->hloc_cnt; i++) {
				if(!strcmp(wp->hloc[i], hloc))
					break;
			}
			if(i == wp->hloc_cnt && wp->hloc_cnt < 10) {
				strcpy(wp->hloc[wp->hloc_cnt], hloc);
				strcpy(wp->num[wp->hloc_cnt], num);
				wp->how[wp->hloc_cnt] = how;
				wp->hloc_cnt++;
			}
			wp->cnt++;
			return;
		}

		if(result > 0)
			break;

		if(wp->next == NULL) {
			wp->next = allocate(NULL);
			wp->next->last = wp;
			strcpy(wp->next->call, call);
			strcpy(wp->next->hloc[0], hloc);
			strcpy(wp->next->num[0], num);
			wp->next->how[0] = how;
			wp->next->hloc_cnt++;
			wp->next->cnt++;
			return;
		}

		wp = wp->next;
	} 

	if(wp == wpl) {
		nwp = allocate(NULL);
		nwp->next = wpl;
		wpl->last = nwp;
		wpl = nwp;
	} else
		nwp = allocate(wp);

	strcpy(nwp->call, call);
	strcpy(nwp->hloc[0], hloc);
	strcpy(nwp->num[0], num);
	nwp->how[0] = how;
	nwp->hloc_cnt++;
	nwp->cnt++;
}

static void
scan_file(FILE *fp, char *num)
{
	char *p, *hloc, *call, buf[1024];

	while(fgets(buf, 1024, fp)) {
		struct tm tm;
		if(buf[0] != 'R')
			return;

		if((p = (char*)index(buf, '@')) == NULL) {
			fprintf(stderr, "Bad @\t%s\n", buf);
			continue;
		}

		while(!isalnum(*p)) p++;
		call = p;
		while(isalnum(*p)) p++;
		*p++ = 0;

		hloc = p;
		while(!isspace(*p)) p++;
		*p = 0;

		if(strlen(hloc) > 1)
			check_list(WPBBS_FromHeader, call, hloc, num);
	}
}

void
output_list(FILE *fp)
{
	struct wp_list *wp = wpl;
	int cnt = 0;

	while(wp) {
		int i = 0;

		if(wp->hloc_cnt > 1) {
			char buf[80];

			printf("%s\n", wp->call);
			for(i=0; i<wp->hloc_cnt; i++)
				printf("%d:\t%c %s\n", i, wp->how[i], wp->hloc[i]);
			gets(buf);

			i = atoi(buf);
		}

		fprintf(fp, "%c %s\t%s\n", wp->how[i], wp->call, wp->hloc[i]);
		cnt++;
		wp = wp->next;
	}

	printf("\ntotal count = %d\n", cnt);
}

void
append(char *out, char *in, int flag)
{
	if(*in == 0)
		return;

	if(*out)
		strcat(out, ".");
	if(flag)
		strcat(out, "#");
	strcat(out, in);
}

main()
{
	struct dirent *dp;
	DIR *dirp;
	FILE *fp;
	char filename[80];
	struct wp_bbs_entry wp;

	dirp = opendir(MSGBODYPATH);
	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
		if(dp->d_name[0] == '.' || islower(dp->d_name[0]))
			continue;

		sprintf(filename, "%s/%s", MSGBODYPATH, dp->d_name);
		if((fp = fopen(filename, "r")) == NULL)
			printf("%s:\t[Failure opening file]\n", dp->d_name);
		else {
			if(!strcmp(dp->d_name, "23666")) {
				int i = 10;
			}
			scan_file(fp, dp->d_name);
			fclose(fp);
		}
	}

	closedir(dirp);
		/* now load old database */

	if((fp = fopen("/bbs/wp/bbs", "r")) == NULL) {
		printf("Couldn't open old wp/bbs file\n");
		exit(1);
	}

	while(fread(&wp, SizeofWpBbs, 1, fp)) {
		char *p, hloc[LenHLOC];
		int cnt;

		hloc[0] = 0;
		append(hloc, wp.locale[0], 1);
		append(hloc, wp.locale[1], 1);
		append(hloc, wp.addr[0], 0);
		append(hloc, wp.addr[1], 0);
		append(hloc, wp.addr[2], 0);

		cnt = 0;
		p = wp.call;
		while(*p) {
			if(!isalnum(*p++)) {
				cnt = 0;
				break;
			}
			if(++cnt > 6)
				break;
		}
		if(cnt < 3 || cnt > 6)
			continue;

		cnt = strlen(hloc);
		if(cnt < 2 || cnt > LenHLOC)
			continue;

		check_list(WPBBS_FromWPupdate, wp.call, hloc, "0");
	}
	fclose(fp);	

	if((fp = fopen(WPBBSFILE, "w")) == NULL) {
		printf("Couldn't open %s for writing\n", WPBBSFILE);
		exit(1);
	}

	output_list(fp);
	fclose(fp);
	exit(0);
}
