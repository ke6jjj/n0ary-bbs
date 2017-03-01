
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "body.h"
#include "tokens.h"
#include "vars.h"

struct body_line {
	struct body_line *next;
	char buf[256];
} *FirstBodyLine = NULL;

static struct body_line
	*allocate_line(char *str, struct body_line *previous);

static void
	write_body(struct text_line **tl),
	free_body(void),
	insert_line(int num, char *str),
	kill_line(int num),
	edit_line(int num),
	list_body(void);

static int
	include_signature(void);

/*=====================================================================
 * This file contains routines to create generic bodies for messages and
 * files. There are some context sensitive issues that differ between
 * these.
 *
 * These bodies are stored in a linked list of lines. this allows the
 * user to have some crude editing capabilities, adding lines, deleteing
 * lines, altering lines, etc.
 *
 * This routine is used to allocate a new line whenever required. The
 * following function is used to free up the body list when it is no
 * longer needed.
 */

static struct body_line *
allocate_line(char *str, struct body_line *previous)
{
	struct body_line *line = malloc_struct(body_line);
	int len = strlen(str);

	if(previous)
		previous->next = line;

	if(len > 255) {
		strncpy(line->buf, str, 255);
		line->buf[255] = 0;
		allocate_line(&str[255], line);
	} else
		strcpy(line->buf, str);
	return line;
}

static void
write_body(struct text_line **tl)
{
	struct body_line *line = FirstBodyLine;

	while(line) {
		if(line->buf[strlen(line->buf)-1] == '\n')
			line->buf[strlen(line->buf)-1] = 0;
		textline_append(tl, line->buf);	
		NEXT(line);
	}
	free_body();
}

static void
free_body(void)
{
	struct body_line *tmp = FirstBodyLine;
	while(FirstBodyLine) {
		tmp = FirstBodyLine;
		NEXT(FirstBodyLine);
		free(tmp);
	}
}

/*======================================================================
 * Add a new line to the end of the body list.
 */

void
link_line(char *str)
{
	struct body_line *line = FirstBodyLine;


	if(line) {
		while(line->next)
			NEXT(line);
 		allocate_line(str, line);
	} else
		FirstBodyLine = allocate_line(str, NULL);
}

/*======================================================================
 * Insert this line just prior to the line number specified.
 */

static void
insert_line(int num, char *str)
{
	struct body_line *link, *line = FirstBodyLine;

	if(num == 1) {
		link = FirstBodyLine;
		FirstBodyLine = NULL;
		link_line(str);
		line = FirstBodyLine;
	} else {
		num--;
		while(--num && line->next)
			NEXT(line);

		link = line->next;
		allocate_line(str, line);
	}

	while(line->next)
		NEXT(line);

	line->next = link;
}

/*======================================================================
 * Kill the line supplied
 */

static void
kill_line(int num)
{
	struct body_line *tmp, *line = FirstBodyLine;

	if(num == 1) 
		FirstBodyLine = FirstBodyLine->next;
	else {
		while(--num && line->next) {
			tmp = line;
			NEXT(line);
		}
		if(num)
			return;
		tmp->next = line->next;
	}

	free(line);
}

/*======================================================================
 * Show the user the current line at the line number supplied then
 * accept it's replacement. If the user hits a return without entering
 * a replacement, don't change the line.
 */

static void
edit_line(int num)
{
	char buf[256];
	int lnum = num;
	struct body_line *line = FirstBodyLine;

	while(--num && line->next) 
		NEXT(line);

	if(num)
		return;

	PRINT(line->buf);
	GETS(buf, 255);

	if(buf[0]) {
		kill_line(lnum);
		strcat(buf, "\n");
		insert_line(lnum, buf);
	}
}

/*======================================================================
 * Show the user the current state of his body. The lines are displayed
 * with line numbers on the left side. These line numbers are for use
 * in the editing commands.
 */

static void
list_body(void)
{
	struct body_line *line = FirstBodyLine;
	int num = 1;
	char buf[256];

		/* QUESTION: We have a limitation of 99 lines here. Is that
		 * going to be enough?
		 */

	while(line) {
		sprintf(buf, "%2d:%s", num++, line->buf);
		PRINT(buf);
		NEXT(line);
	}
}

/*=========================================================================
 * This routine looks at the supplied line and looks for a valid command
 * to exist in the first X positions. This is context sensitive in that
 * certain commands are not valid for filesys write or message send.
 */

int
msg_line_type(char *str, int type)
{
	char buf[4096];

		/* If we are a bbs then this has to be a message and we should
		 * check for routing info and the approved exits. NOTE: that
		 * the unix exits are not allowed here.
		 */

	if(ImBBS) {
		switch(*str) {
		case '/':
			case_strcpy(buf, str, AllUpperCase);
			if(!strncmp(buf, "/EX", 3))
				return mTERM;
			break;

		case 'R':
			str++;
			if(*str == ':')
				return mROUTING;
			break;
	
		case '':
			return mTERM;
		}

		if((str = (char*)index(str, ''))!= NULL) {
			if(*(str+1) == 0 || *(str+1) == '\n') {
				*str = 0;
				return mTERMwrite;
			}
		}
		return OK;
	}

		/* Force uppercase to aid in compares */

	case_strcpy(buf, str, AllUpperCase);

		/* if we are an average user then we have the editor at our
		 * disposal. But there is a difference in what commands are
		 * available for file write and message send.
		 */

	if(type == MSG_BODY)
		switch(*str) {
		case '/':
			if(!strncmp(buf, "/IN", 3))
				return mINCLUDE;
			if(!strncmp(buf, "/CC", 3))
				return mCC;
			if(!strncmp(buf, "/SI", 3))
				return mSIGNATURE;
			if(!strncmp(buf, "/NOS", 4))
				return mNOSIGNATURE;
			break;
		case 'R':
			str++;
			if(*str == ':')
				return mROUTING;
			break;
		}

		/* Of course sysops are allowed more previledges in the file
		 * write routines.
		 */

	if(ImSysop && *str == '/') {
		if(!strncmp(buf, "/IN", 3))
			return mINCLUDE;
		if(!strncmp(buf, "/RF", 3))
			return mREADFILE;
		if(!strncmp(buf, "/SI", 3))
			return mSIGNATURE;
		if(!strncmp(buf, "/NOS", 4))
			return mNOSIGNATURE;
	}
		
		/* The following are generic, allowed during message send or
		 * file write. These are basically just the editor and termination
		 * commands.
		 */

	switch(*str) {
	case '/':
		if(!strncmp(buf, "/EX", 3))
			return mTERM;
		if(!strncmp(buf, "/AB", 3))
			return mABORT;
		if(!strncmp(buf, "/NU", 3))
			return mLISTNUMBERS;
		if(!strncmp(buf, "/ED", 3))
			return mEDITLINE;
		if(!strncmp(buf, "/KI", 3))
			return mKILLLINE;
		if(!strncmp(buf, "/AD", 3))
			return mADDLINE;
		if(!strncmp(buf, "/HE", 3))
			return mHELP;
		if(!strncmp(buf, "/?", 2))
			return mHELP;
		break;
	
	case '.':
		str++;
			/* The period has to be the only thing on the line to count
			 * as a termination character.
			 */
		if(*str)
			return OK;
	case '':
		return mTERM;
	}
	
		/* Ok, so the line didn't start with a command. There is one other
		 * thing to check for, is there a ^Z somewhere else on the line.
		 * If so then instruct to terminate after writing the line. The
		 * ^Z is striped now.
		 *
		 * QUESTION: Should we be checking that the ^Z was the last character
		 * on the line? Currently we truncate from there to the end.
		 */
	if((str = (char*)index(str, ''))!= NULL) {
		if(*(str+1) == 0) {
			*str = 0;
			return mTERMwrite;
		}
	}

		/* nothing of interest here, just a normal line of text */
	return OK;
}

/*=========================================================================
 * prompt the user for the body or either a message or a file. The type
 * parameter (MSG_BODY vs FILE_BODY) is used to determine which commands
 * are allowed.
 */
	
int
get_body(struct text_line **tl, int type, char *orig_bbs,
	int *orig_num, time_t *orig_date)
{
	char *p, buf[4096];
	int signature = FALSE;
	int in_routing = TRUE;

	while(TRUE) {
		int mltype;

		GETS(buf, 4095);
		mltype = msg_line_type(buf, type);

			/* see if the line contains a command. Commands that are
			 * not valid in a given context (ie. signature in a file)
			 * will be taken into account in the msg_line_type() routine
			 * and need not be checked here.
			 */

		switch(mltype) {
		case mABORT:
			free_body();
			return mABORT;

		case mINCLUDE:
			include_msg(buf, type);
			continue;

		case mREADFILE:
			include_file(buf);
			continue;

		case mSIGNATURE:
			include_signature();
		case mNOSIGNATURE:
			signature = TRUE;
			continue;

		case mHELP:
			switch(type) {
			case FILE_BODY:
			case EVENT_BODY:
				system_msg(996);
				break;
			case MSG_BODY:
				system_msg(991);
				break;
			}
			continue;

		case mADDLINE:
			{
				int i;
				p = &buf[3];
				NextSpace(p);
				NextChar(p);
				i = get_number(&p);

				GETS(buf, 4095);
				strcat(buf, "\n");
				insert_line(i, buf);
			}
			continue;
			
		case mEDITLINE:
			p = &buf[3];
			NextSpace(p);
			NextChar(p);
			edit_line(get_number(&p));
			continue;

		case mKILLLINE:
			p = &buf[3];
			NextSpace(p);
			NextChar(p);
			kill_line(get_number(&p));
			continue;

		case mLISTNUMBERS:
			list_body();
			continue;

		case mCC:
			if(AutoSignature && signature == FALSE)
				include_signature();
			link_line("\n");
			if(!ImBBS)
				insert_line(1, "\n");
			write_body(tl);
			return mCC;

		case mROUTING:
			if(in_routing) {
				read_routing(&buf[2], orig_bbs, orig_date, orig_num);
				strcat(buf, "\n");
				link_line(buf);
				continue;
			}
			break;

		case mTERMwrite:
			if(type == DISTRIB_BODY)
				case_string(buf, AllUpperCase);
			strcat(buf, "\n");
			link_line(buf);
					/* FALLTHROUGH */
		case mTERM:
			switch(type) {
			case EVENT_BODY:
				if(!ImSysop) {
					time_t t = Time(NULL);
					struct tm *tm = localtime(&t);
					char datebuf[40];
					strftime(datebuf, 40, "%D", tm);
					sprintf(buf, "\n# %s %s\n", datebuf, usercall);
					link_line(buf);
				}
				break;

			case FILE_BODY:
				if(!ImSysop) {
					sprintf(buf, "\n#================================");
					link_line(buf);
					sprintf(buf, "\n# Submitted by %s\n", usercall);
					link_line(buf);
				}
				break;

			case MSG_BODY:
				if(AutoSignature && signature == FALSE)
					include_signature();
				break;

			case DISTRIB_BODY:
				sprintf(buf, "#! %s\n", usercall);
				link_line(buf);
				PRINT("List calls of people allowed to alter this list. As the creator your\n");
				PRINTF("call, %s, has already been entered. Type /EX to terminate the list.\n\n", usercall);

				while(TRUE) {
					char name[80];
					GETS(name, 79);
					if(name[0] == 0)
						continue;
					uppercase(name);
					if(!strncmp(name, "/EX", 3))
						break;
					sprintf(buf, "#! %s\n", name);
					link_line(buf);
				}
				break;
			}

			if(!ImBBS)
				insert_line(1, "\n");
			write_body(tl);
			return mTERM;
		}

		in_routing = FALSE;
		if(type == DISTRIB_BODY)
			case_string(buf, AllUpperCase);

		strcat(buf, "\n");
		link_line(buf);
	}
}

/*===========================================================================
 * Each user has the option of having a .signature file. This routine will
 * append it to the current message body, where ever it currently is.
 */

static int
include_signature(void)
{
	FILE *fpr;
	char str[80];

		/* open the users file, if it exists */

	sprintf(str, "%s/%s", Bbs_Sign_Path, usercall);
	if((fpr = fopen(str, "r")) == NULL)
		return system_msg(995);

		/* put it in the current message */

		/* code can identify the begining of a signature by a line that
		 * contains only three spaces.
		 */

	link_line("   \n");

	while(fgets(str, 80, fpr)) {
		link_line(str);
		PRINTF("%s", str);
	}
	fclose(fpr);
	return OK;
}

/*===========================================================================
 * Include a file from somewhere in the unix filesystem and put it in the
 * message at the current location. This should be a sysop only command.
 * No one else should have knowledge of the unix filesystem.
 *
 * QUESTION: This does mean that users cannot include files from the bbs
 * file system in messages. Is this ok?
 */

int
include_file(char *buf)
{
	FILE *fpr;
	char str[1024];

		/* increment pointer to filename */

	NextSpace(buf);
	NextChar(buf);
	
		/* and open the file */

	strcpy(str, get_string(&buf));
	if((fpr = fopen(str, "r")) == NULL)
		return error_string(190, "opening");

		/* copy the file to the body at current position. This also 
		 * displays it to the users screen.
		 */

	while(fgets(str, 1023, fpr)) {
		str[1023] = 0;
		link_line(str);
		PRINTF("%s", str);
	}

	link_line("\n");
	PRINT("\n");
	fclose(fpr);
	return OK;
}

