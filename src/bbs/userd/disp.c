#include <stdio.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

static int
is_number(char *s)
{
	while(*s)
		if(!isdigit(*s++))
			return FALSE;
	return TRUE;
}

static char *
disp_last(int num)
{
	struct UserDirectory *dir = UsrDir;

	output[0] = 0;
	while(dir) {
		sprintf(output, "%s%.6s\t%5d\t%s\t%"PRTMd"\n", output, dir->call,
			dir->connect_cnt, port_name(dir->port), dir->lastseen);

		if(--num == 0 || strlen(output) > 4000)
			break;
		NEXT(dir);
	}
	strcat(output, ".\n");
	return output;
}

char *
disp_user(char *s)
{
	struct UserDirectory *dir = UsrDir;
	char call[LenCALL];

	if(*s == 0)
		return Error("expected callsign or count");

	strcpy(call, get_string(&s));
	uppercase(call);

	if(is_number(call))
		return disp_last(atoi(call));

	if(strlen(call) > 6)
		return Error("call too long");

	while(dir) {
		if(!strcmp(dir->call, call)) {
			sprintf(output, "%.6s\t%5d\t%s\t%"PRTMd"\n",
				call, dir->connect_cnt, port_name(dir->port), dir->lastseen);
			return output;
		}
		NEXT(dir);
	}
	return Error("no such user");
}	
