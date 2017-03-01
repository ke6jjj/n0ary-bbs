#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "distrib.h"

#include "user.h"
#include "tokens.h"
#include "body.h"
#include "vars.h"

static int distrib_list(void);
static int distrib_kill(char *fn);
static int distrib_write(char *fn);
static int distrib_read(char *fn);
static int distrib_approve(char *fn, FILE *fp);

int
distrib_t(void)
{
	struct TOKEN *t = TokenList;
	int function = LIST;
	char name[80];

	NEXT(t);
	name[0] = 0;

	while(t->token != END) {
		switch(t->token) {
		case KILL:
			function = KILL;
			break;
		case WRITE:
			function = WRITE;
			break;
		case READ:
			function = READ;
			break;
		case WORD:
			strcpy(name, t->lexem);
			break;
		case LIST:
			break;
		default:
			bad_cmd(-1, t->location);
			return ERROR;
		}
		NEXT(t);
	}

	switch(function) {
	case KILL:
		if(name[0] == 0) {
			distrib_list();
			PRINT("Which list do you wish to KILL? ");
			if(NeedsNewline)
				PRINT("\n");
			GETS(name, 79);
			if(name[0] == 0)
				return OK;
		}

		case_string(name, AllUpperCase);
		distrib_kill(name);
		break;

	case WRITE:
		if(name[0] == 0) {
			PRINT("To create a distribution list you will be ask for 4 things:\n");
			PRINT("  1. name, typically club initials, no spaces and 6 characters max\n");
			PRINT("  2. title, description of list\n");
			PRINT("  3. members\n");
			PRINT("  4. calls of those allowed to kill the list\n\n");
			PRINT("Distribution name? ");
			if(NeedsNewline)
				PRINT("\n");
			GETS(name, 79);
			if(name[0] == 0)
				return OK;
			name[6] = 0;
		}
		
		case_string(name, AllUpperCase);
		distrib_write(name);
		break;

	case READ:
		if(name[0] == 0) {
			distrib_list();
			PRINT("Which list do you wish to read? ");
			if(NeedsNewline)
				PRINT("\n");
			GETS(name, 79);
			if(name[0] == 0)
				return OK;
		}
		
		case_string(name, AllUpperCase);
		distrib_read(name);
		break;

	case LIST:
		distrib_list();
		break;
	}

	return OK;
}

static int
distrib_kill(char *fn)
{
	FILE *fp;
	char filename[80];
	char *p, buf[4096];

	sprintf(filename, "%s/%s", Bbs_Distrib_Path, fn);

	if((fp = fopen(filename, "r")) == NULL) {
		fclose(fp);
		PRINTF("The distribution list %s doesn't currently exist.\n", fn);
		PRINTF("The following lists are currently available\n\n");
		distrib_list();
		return OK;
	}

	fgets(buf, 80, fp);

	p = buf;
	if(*p != '#') {
		PRINTF("Distribution format is invalid, notify a sysop.\n");
		PRINTF("KILL DISTRIBUTION command aborted\n");
		return ERROR;
	}

	p++;
	PRINTF("%s: %s\n", fn, p);
	PRINTF("This this the list you wish to kill (y/N)?");

	if(get_yes_no(NO) == YES) {
		if(!ImSysop)
			if(distrib_approve(fn, fp) == ERROR) {
				fclose(fp);
				return ERROR;
			}
		fclose(fp);
		unlink(filename);
		return OK;
	}

	fclose(fp);
	return OK;
}

static int
distrib_approve(char *fn, FILE *fp)
{
	char buf[80];
	char allow_list[4096];

	allow_list[0] = 0;

	while(fgets(buf, 80, fp)) {
		if(!strncmp(buf, "#! ", 3)) {
			char *p = (char *)index(buf, '\n');
			*p = 0;

			sprintf(allow_list, "%s %s", allow_list, &buf[3]);

			if(!strcmp(&buf[3], usercall)) {
				rewind(fp);
				return OK;
			}
		}
	}

	PRINTF("\nOnly the creator and a list of people supplied by the creator\n");
	PRINTF("are allowed to KILL a distribution list.\n");
	PRINTF("Contact sysop for help. Here are the approved people for %s\n", fn);
	PRINTF("%s\n", allow_list);

	rewind(fp);
	return ERROR;
}

static int
distrib_read(char *fn)
{
	FILE *fp;
	char filename[80];
	char *p, buf[4096];

	sprintf(filename, "%s/%s", Bbs_Distrib_Path, fn);

	if((fp = fopen(filename, "r")) == NULL) {
		fclose(fp);
		PRINTF("The distribution list %s doesn't currently exist.\n", fn);
		PRINTF("The following lists are currently available\n\n");
		distrib_list();
		return OK;
	}

	fgets(buf, 80, fp);
	p = buf;
	if(*p != '#') {
		PRINTF("Distribution format is invalid, notify a sysop.\n");
		PRINTF("DISTRIBUTION READ command aborted\n");
		return ERROR;
	}

	rewind(fp);

	fgets(buf, 80, fp);
	PRINTF("\n%s", &buf[1]);

	while(fgets(buf, 80, fp)) {
		if(buf[0] == '#')
			break;
		PRINTF("  %s", buf);
	}

	PRINTF("\nThe list can be killed by the following people:\n");

	do {
		PRINTF("  %s", &buf[3]);
	} while(fgets(buf, 80, fp));

	PRINT("\n");

	fclose(fp);
	return OK;
}

static int
distrib_list(void)
{
	struct dirent *dp;
	DIR *dirp;
	FILE *fp;
	char filename[80];

	dirp = opendir(Bbs_Distrib_Path);
	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
		if(dp->d_name[0] == '.' || islower(dp->d_name[0]))
			continue;

		sprintf(filename, "%s/%s", Bbs_Distrib_Path, dp->d_name);
		if((fp = fopen(filename, "r")) == NULL)
			PRINTF("%s:\t[Failure opening file]\n", dp->d_name);
		else {
			char buf[256], *p = buf;
			char str[256];

			fgets(buf, 80, fp);
			fclose(fp);
			
			if(*p++ != '#')
				p = "[File format error]";

			sprintf(str, "%s:", dp->d_name);
			strcat(str, "                    ");
			str[16] = 0;
			strcat(str, p);

			PRINT(str);
		}
	}

	closedir(dirp);
	return OK;
}

static int
distrib_write(char *fn)
{
	FILE *fp;
	char *name;
	char filename[80];
	char buf[4096];
	int result = ERROR;
	struct text_line *tl = NULL, *t;

	name = get_string(&fn);
	sprintf(filename, "%s/%s", Bbs_Distrib_Path, name);

	if((fp = fopen(filename, "r")) != NULL) {
		fclose(fp);
		PRINTF("\nThe distribution list %s already exists, this must be killed\n", name);
		PRINTF("prior to creating a new list. See DISTRIBUTION KILL\n");
		return ERROR;
	}

	if((fp = fopen(filename, "w")) == NULL) {
		PRINTF("\nFailure opening distribution list as %s.\n", filename);
		PRINT("Please notify the sysop of the problem\n");
		return ERROR;
	}

	PRINT("\nSo that others can determine who this list represents please supply\n");
	PRINT("short (70 character max) description of this list. This will typically be\n");
	PRINT("the club name spelled out, rather than abbreviated:\n");

	GETS(buf, 4095);
	fprintf(fp, "#%s\n", buf);

	PRINT("\nNow enter the calls of the people you wish to have on this list. If you\n");
	PRINT("you don't supply a home bbs this bbs will automatically address according\n");
	PRINT("to the receipents WP entry. Type a /EX to terminate the list.\n\n");

	if(get_body(&tl, DISTRIB_BODY, NULL, NULL, NULL) == mTERM)
		result = OK;
	t = tl;
	while(t) {
		if(t->s[0])
			fprintf(fp, "%s\n", t->s);
		NEXT(t);
	}
	textline_free(tl);

	fclose(fp);

	if(result == ERROR)
		unlink(filename);

	PRINTF("Distribution list %s created\n", name);
	return OK;
}

static FILE *dfp = NULL;

int
distrib_open(char *fn, char *title)
{
	char *p, str[80];

	case_string(fn, AllUpperCase);
	sprintf(str, "%s/%s", Bbs_Distrib_Path, fn);
   
	if((dfp = fopen(str, "r")) == NULL) {
		PRINTF("No distribution list named %s, type DIST LIST to see choices.\n", fn);
		return ERROR;
	}

	fgets(str, 80, dfp);
	p = (char *)index(str, '\n');
	*p = 0;

	strcpy(title, &str[1]);
	return OK;
}

int
distrib_get_next(char *call)
{
	char *p, str[80];
	fgets(str, 80, dfp);

	if(str[0] == '#') {
		distrib_close();
		return ERROR;
	}

	p = (char *)index(str, '\n');
	*p = 0;

	if(str[0] == 0)
		return distrib_get_next(call);

	strcpy(call, str);
	return OK;
}

int
distrib_close(void)
{
	if(dfp != NULL) {
		fclose(dfp);
		dfp = NULL;
	}
	return OK;
}
