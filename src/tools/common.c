#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "c_cmmn.h"
#include "tools.h"

int
IsPrintable(char *s)
{
	while(*s) {
		if(!isprint(*s))
			return FALSE;
		s++;
	}
	return TRUE;
}

short
sum_string(char *s)
{
	short sum = 0;
	while(*s)
		sum += (short)*s++;
	return sum;
}

void
uppercase(char *s)
{
	while(*s) {
		ToUpper(*s);
		s++;
	}
}

void
lowercase(char *s)
{
	while(*s) {
		ToLower(*s);
		s++;
	}
}

void
kill_trailing_spaces(char *s)
{
	int i;
	for(i = strlen(s)-1; i; i--) {
		if(!isspace(s[i]))
			return;
		s[i] = 0;
	}
}

int
stricmp(char *s1, char *s2)
{
	char p1[256], p2[256];
	strcpy(p1, s1);
	strcpy(p2, s2);
	uppercase(p1);
	uppercase(p2);
	return strcmp(p1, p2);
}

char *
get_call(char **str)
{
	static char buf[256];
	char *p = buf;
	buf[0] = 0;

	if(**str) {
		while(isalnum(**str) && **str)
			*p++ = *(*str)++;
		*p = 0;
		NextChar(*str);
	} else
		return NULL;
	return buf;
}

char *
get_string(char **str)
{
	static char buf[1024];
	char *p = buf;
	buf[0] = 0;

	if(**str == '"') {
		(*str)++;
		while(**str != '"' && **str)
			*p++ = *(*str)++;
		*p = 0;
		(*str)++;
		NextChar(*str);
		return buf;
	}

	if(**str) {
		while(!isspace(**str) && **str)
			*p++ = *(*str)++;

		*p = 0;
		NextChar(*str);
	} else
		return NULL;
	return buf;
}	


long
get_hexnum(char **str)
{
    char buf[20], *p = buf;
    long num;

		/* check for an optional "0x" and skip over */
	if(**str == '0' && (*(*str+1) == 'x' || *(*str+1) == 'X'))
		(*str) += 2;

    while(isxdigit(**str))
        *p++ = *(*str)++;
    *p = 0;
    NextChar(*str);

    if(buf[0] == 0)
        return ERROR;

    sscanf(buf, "%lx", &num);
    return num;
}

long
get_number(char **str)
{
    char buf[20], *p = buf;
    long num;
	int neg = FALSE;

	if(**str == '-') {
		neg = TRUE;
		(*str)++;
	}

    while(isdigit(**str))
        *p++ = *(*str)++;
    *p = 0;
    NextChar(*str);

    if(buf[0] == 0)
        return ERROR;

    sscanf(buf, "%ld", &num);

	if(neg)
		num *= -1;
    return num;
}

char *
get_word(char **str)
{
	static char buf[256];
	char *p = buf;
	buf[0] = 0;

	if(**str) {
		while(isalpha(**str) && **str)
			*p++ = *(*str)++;
		*p = 0;
		NextChar(*str);
	} else
		return NULL;
	return buf;
}	

char *
get_string_to(char **str, char term)
{
	static char buf[256];
	char *p = buf;
	buf[0] = 0;

	if(**str) {
		while((**str != term) && **str)
			*p++ = *(*str)++;

		*p = 0;
#if 0
		(*str)++;
		NextChar(*str);
#endif
	} else
		return NULL;
	return buf;
}	

char *
copy_string(char *s)
{
	char *c = (char*)malloc(strlen(s)+1);
	strcpy(c, s);
	return c;
}

void *
mem_calloc(int cnt, int size)
{
	void *p = (void*)calloc(cnt, size);
	if(p == NULL) {
		printf("memory allocation failure!!!!\n");
		exit(1);
	}
	return (void *)p;
}

