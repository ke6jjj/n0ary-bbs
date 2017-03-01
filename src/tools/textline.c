#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "c_cmmn.h"
#include "tools.h"

struct text_line *
textline_allocate(char *s)
{
	struct text_line *tl;

	if(s == NULL)
		return;

	tl = malloc_struct(text_line);
	tl->s = (char *)malloc(strlen(s)+1);
	tl->next = NULL;
	strcpy(tl->s, s);
	return tl;
}

void
textline_append(struct text_line **tl, char *s)
{
	if(s == NULL)
		return;
	while(*tl)
		tl = &(*tl)->next;
	*tl = textline_allocate(s);
}

void
textline_prepend(struct text_line **tl, char *s)
{
	struct text_line *tmp_tl = *tl;

	if(s == NULL)
		return;

	*tl = textline_allocate(s);
	(*tl)->next = tmp_tl;
}

struct text_line *
textline_free(struct text_line *tl)
{
	while(tl) {
		struct text_line *t = tl;
		NEXT(tl);
		if(t->s != NULL)
			free(t->s);
		free(t);
	}
	return NULL;
}

void
textline_sort(struct text_line **tl, int position)
{
	int chg;

	do {
		struct text_line **t = tl;
		chg = 0;

		while((*t)->next) {
			if(strcmp(&((*t)->s[position]), &((*t)->next->s[position])) > 0) {
				struct text_line *tmp = *t;
				*t = (*t)->next;
				tmp->next = (*t)->next;
				(*t)->next = tmp;
				chg++;
			}
			t = &((*t)->next);
		}
	} while(chg);
}

int
textline_maxlength(struct text_line *tl)
{
	int maxlen = 0;
	while(tl) {
		int len = strlen(tl->s);
		if(len > maxlen)
			maxlen = len;
		NEXT(tl);
	}
	return maxlen;
}

int
textline_count(struct text_line *tl)
{
	int cnt = 0;
	while(tl) {
		cnt++;
		NEXT(tl);
	}
	return cnt;
}
