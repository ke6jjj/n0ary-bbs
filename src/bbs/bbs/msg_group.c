#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "vars.h"
#include "pending.h"
#include "rfc822.h"

#define cSEPARATOR	':'
#define cTO			'>'
#define cFROM		'<'
#define cAT			'@'
#define cBID		'$'
#define cSUBJECT	'&'
#define cCOMMAND	'!'
#define cFLAG		'|'
#define cAGE		'+'

extern int
	debug_level;

#ifdef NOMSGD
int
determine_groups(struct message_directory_entry *m)
{
	FILE *fp;
	int found = FALSE;
	char buf[256];

	if(!IsMsgBulletin(m)) {
		if(debug_level & DBG_MSGTRANS)
			PRINTF("GRP: not a bulletin\n");
		return FALSE;
	}

	if((fp = fopen(GROUPFILE, "r")) == NULL)
		return FALSE;

	while(fgets(buf, 256, fp)) {
		char 
			*match = buf,
			*replace;

			/* if line is a comment or blank skip */

		NextChar(match);
		if(*match == '\n' || *match == ';')
			continue;

			/* a line is made up of the following:
			 *		match_tokens separator replacement_tokens
			 * if we can't find the seperator then skip the line.
			 */

		if((replace = (char*) index(buf, cSEPARATOR)) == NULL)
			continue;

			/* NULL terminate the match string */

		*replace++ = 0;
		NextChar(replace);

		if(message_matches_criteria(match, m, Time(NULL)) == TRUE) {
			if(debug_level & DBG_MSGTRANS)
				PRINTF("GRP ident: %s => %s", match, replace);
			found = TRUE;
			while(*replace && *replace != ';') {
				char *s = get_string(&replace);
				rfc822_append(m->number, rGROUP, s);
				if(m->grp_cnt < MAX_GROUPS)
					strcpy(m->groups[m->grp_cnt++], s);
			}
		}
	}

	if(!found) {
		if(debug_level & DBG_MSGTRANS)
			PRINTF("GRP: default to MISC\n");
		rfc822_append(m->number, rGROUP, "MISC");
		if(m->grp_cnt < MAX_GROUPS)
			strcpy(m->groups[m->grp_cnt++], "MISC");
	}

	fclose(fp);
	return TRUE;
}

struct GroupList {
	struct GroupList *next;
	char name[20];
	int total;
	int new;
} *GrpList = NULL;

static void
group_free_list()
{
	struct GroupList *t;

	while(GrpList) {
		t = GrpList;
		NEXT(GrpList);
		free(t);
	}
}

static void
group_add_list(char *group, int new)
{
	struct GroupList *t = GrpList;

	while(t) {
		if(!strcmp(t->name, group)) {
			t->total++;
			if(new)
				t->new++;
			return;
		}
		NEXT(t);
	}

	t = malloc_struct(GroupList);
	t->next = GrpList;
	GrpList = t;
	strcpy(t->name, group);
	t->total = 1;
	if(new)
		t->new = 1;
}

void
group_make_list(struct message_directory_entry *m, long t0)
{
	int i;
	int new = FALSE;

	if(m == NULL) {
		group_free_list();
		return;
	}

	if(m->cdate > t0)
		new = TRUE;

	for(i=0; i<m->grp_cnt; i++)
		group_add_list(m->groups[i], new);
}

static char disp_buf[85];

static void
space_fill(void)
{
	int i;
	for(i=0; i<79; i++)
		disp_buf[i] = ' ';
	disp_buf[i] = 0;
}

static void
place_string(char *s, int loc)
{
	char *p = &disp_buf[loc];
	while(*s)
		*p++ = *s++;
}

static void
place_number(int num, int loc)
{
	char buf[25];
	sprintf(buf, "%d", num);
	place_string(buf, loc);
}

void
group_display()
{
	struct GroupList *t = GrpList;
	int loc = 0;
	int i;

	space_fill();
	for(i=0; i<3; i++) {
		place_string("Group", (i*25));
		place_string("Cnt", (i*25)+10);
		place_string("New", (i*25)+15);
	}
	PRINTF("%s\n", disp_buf);

	space_fill();
	while(t) {
		place_string(t->name, (loc*25));
		place_number(t->total, (loc*25)+10);
		place_number(t->new, (loc*25)+15);
		if(++loc == 3) {
			PRINTF("%s\n", disp_buf);
			space_fill();
			loc = 0;
		}
		NEXT(t);
	}

	if(loc)
			PRINTF("%s\n", disp_buf);
}
#endif
