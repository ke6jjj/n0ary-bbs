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

/* This function replaces the old "get_number()" logic with
 * a modern version which uses strtol() instead of sscanf().
 *
 * The old version was hard-coded to continue parsing past the number by
 * advancing the cursor to the next non-whitespace character. This version
 * makes such behavior optional, keyed on "advance".
 *
 * Example old behavior: *str = "123 Hello"    -> *str = "Hello", return 123
 * Example old behavior: *str = "123Hello"     -> *str = "Hello", return 123
 * Example new, no adv : *str = "123 Hello"    -> *str = " Hello", return 123
 */
static long
get_number_and_advance(char **str, int advance)
{
	long num;
	char *end;

	num = strtol(*str, &end, 10);
	if (end == *str)
		/* nothing parsed */
		return ERROR;

	if (advance) {
		NextChar(end);
	}

	*str = end;

	return num;
}

long
get_number(char **str)
{
	/* Get a number and move to next word */
	return get_number_and_advance(str, 1);
}

time_t
get_time_t(char **str)
{
	return (time_t) get_number_and_advance(str, 1);
}

int
get_time_interval(char **str, int default_unit, int parse_two, long *result)
{
	char *p;
	long base;

	base = get_number_and_advance(str, parse_two);
	if (base == ERROR)
		return ERROR;

	p = *str;

	switch (*p) {
	case 's':
	case 'S':
		/* seconds */
		base *= 1;
		break;
	case 'm':
	case 'M':
		switch (p[1]) {
		case 'i':
		case 'I':
		 	/* minutes */
			base *= tMin;
			break;
		case 'o':
		case 'O':
		 	/* months */
			base *= tMonth;
			break;
		default:
			return ERROR;
		}
		break;
	case 'h':
	case 'H':
		/* hours */
		base *= tHour;
		break;
	case 'd':
	case 'D':
		/* days */
		base *= tDay;
		break;
	case 'w':
	case 'W':
		/* weeks */
		base *= tWeek;
		break;
	case 'y':
	case 'Y':
		/* years */
		base *= tYear;
		break;
	case '\0':
		/* clean default */
		base *= default_unit;
		break;
	case ' ':
		/* allowable default if one word */
		if (parse_two == 0) {
			base *= default_unit;
		} else {
			return ERROR;
		}
		break;
	default:
		/* unknown unit */
		return ERROR;
		break;
	}

	*result = base;
	return OK;
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

