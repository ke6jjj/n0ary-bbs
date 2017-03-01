#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

void
uppercase(char *s)
{
	while(*s) {
		ToUpper(*s);
		s++;
	}
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
	static char buf[256];
	char *p = buf;
	buf[0] = 0;

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

    sscanf(buf, "%x", &num);
    return num;
}

long
get_number(char **str)
{
    char buf[20], *p = buf;
    long num;

    while(isdigit(**str))
        *p++ = *(*str)++;
    *p = 0;
    NextChar(*str);

    if(buf[0] == 0)
        return ERROR;

    sscanf(buf, "%d", &num);
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
		(*str)++;
		NextChar(*str);
	} else
		return NULL;
	return buf;
}	
