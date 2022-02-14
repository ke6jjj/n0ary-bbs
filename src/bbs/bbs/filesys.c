#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "filesys.h"
#include "user.h"
#include "message.h"
#include "vars.h"
#include "body.h"
#include "bbscommon.h"
#include "msg_gen.h"
#include "msg_read.h"

int Filecnt;
char File[20][80];
char CurDir[80];

int
signature(void)
{
	struct TOKEN *t = TokenList;
	NEXT(t);

	switch(t->token) {
	case WRITE:
		make_signature();
		break;
	case END:
		show_signature();
		break;
	case KILL:
		kill_signature();
		break;
	default:
		PRINT("Expected SIGNATURE WRITE|KILL|ON|OFF\n");
	}
	return OK;
}

void
kill_signature(void)
{
	char buf[81];
	sprintf(buf, "%s/%s", Bbs_Sign_Path, usercall);
	unlink(buf);
	user_clr_flag(uSIGNATURE);
}

int
make_signature(void)
{
	FILE *fp;
	char buf[4096];
	sprintf(buf, "%s/%s", Bbs_Sign_Path, usercall);
	if((fp = fopen(buf, "w")) == NULL) {
		PRINTF("Cannot open signature file\n");
		return ERROR;
	}

	system_msg(994);

	while(TRUE) {
		int done = FALSE;
		GETS(buf, 4095);
		switch(msg_line_type(buf, FILE_BODY)) {
		case mABORT:
			fclose(fp);
			kill_signature();
			return OK;

		case mHELP:
			system_msg(992);
			continue;
		case mTERM:
			done = TRUE;
			break;
		case mTERMwrite:
			fprintf(fp, "%s\n", buf);
			done = TRUE;
			break;
		}

		if(done == TRUE)
			break;
		fprintf(fp, "%s\n", buf);
	}

	fclose(fp);
	return OK;
}

int
show_signature(void)
{
	FILE *fp;
	char buf[81];
	sprintf(buf, "%s/%s", Bbs_Sign_Path, usercall);
	if((fp = fopen(buf, "r")) == NULL) {
		system_msg(995);
		return ERROR;
	}

	while(fgets(buf, 80, fp))
		PRINT(buf);
	fclose(fp);
	if(AutoSignature)
		PRINT("Automatic SIGNATURE is ON\n");
	else
		PRINT("Automatic SIGNATURE is OFF\n");
	return OK;
}

int
vacation(void)
{
	struct TOKEN *t = TokenList;
	NEXT(t);

	switch(t->token) {
	case WRITE:
		make_vacation();
		break;
	case END:
		show_vacation();
		break;
	case KILL:
		kill_vacation();
		break;
	default:
		PRINT("Expected VACATION WRITE|KILL|ON|OFF\n");
	}
	return OK;
}

void
kill_vacation(void)
{
	char buf[81];
	sprintf(buf, "%s/%s", Bbs_Vacation_Path, usercall);
	unlink(buf);
	user_clr_flag(uVACATION);
}

int
make_vacation(void)
{
	FILE *fp;
	char buf[4096];
	sprintf(buf, "%s/%s", Bbs_Vacation_Path, usercall);
	if((fp = fopen(buf, "w")) == NULL) {
		PRINTF("Cannot open vacation file\n");
		return ERROR;
	}

	system_msg(101);

	while(TRUE) {
		int done = FALSE;
		GETS(buf, 4095);
		switch(msg_line_type(buf, FILE_BODY)) {
		case mABORT:
			fclose(fp);
			kill_vacation();
			return OK;

		case mHELP:
			system_msg(992);
			continue;
		case mTERM:
			done = TRUE;
			break;
		case mTERMwrite:
			fprintf(fp, "%s\n", buf);
			done = TRUE;
			break;
		}

		if(done == TRUE)
			break;
		fprintf(fp, "%s\n", buf);
	}

	fclose(fp);
	return OK;
}

int
show_vacation(void)
{
	FILE *fp;
	char buf[81];
	sprintf(buf, "%s/%s", Bbs_Vacation_Path, usercall);
	if((fp = fopen(buf, "r")) == NULL) {
		system_msg(103);
		return ERROR;
	}

	while(fgets(buf, 80, fp))
		PRINT(buf);
	fclose(fp);
	if(AutoVacation)
		PRINT("Automatic VACATION is ON\n");
	else
		PRINT("Automatic VACATION is OFF\n");
	return OK;
}

int
vacation_file_exists(void)
{
	FILE *fp;
	char buf[81];
	sprintf(buf, "%s/%s", Bbs_Vacation_Path, usercall);
	if((fp = fopen(buf, "r")) == NULL)
		return FALSE;
	fclose(fp);
	return TRUE;
}

int
signature_file_exists(void)
{
	FILE *fp;
	char buf[81];
	sprintf(buf, "%s/%s", Bbs_Sign_Path, usercall);
	if((fp = fopen(buf, "r")) == NULL)
		return FALSE;
	fclose(fp);
	return TRUE;
}

int
filesys(void)
{
	struct TOKEN *t = TokenList;
	int token = t->token;
	NEXT(t);

	switch(token) {
	case CD:
		case_string(&CmdLine[t->location], AllUpperCase);
		file_cd(t);
		break;

	case DIRECTORY:
		case_string(&CmdLine[t->location], AllUpperCase);
		file_ls(t);
		break;

	case WRITE:
		file_write_t(t);
		break;

	case FILESYS:
		if(t->token == APPROVED)
			file_approve(t->next);
		break;
	}
	return OK;
}

void
file_init(void)
{
	chdir(Bbs_FileSys_Path);	
	getcwd(CurDir, 80);
	system("rm -f core");
}

int
file_cd(struct TOKEN *t)
{
	char *d;

	if(t->token == END) {
		PRINT(CurDir);
		return OK;
	}

	d = &CmdLine[t->location];
	while(*d) {
		if(*d == '\\')
			*d = '/';
		d++;
	}

	d = &CmdLine[t->location];
	if(*d == '/') {
		file_init();
		return OK;
	}

	chdir(d);
	getcwd(CurDir, 80);
	return OK;
}

int
file_ls(struct TOKEN *t)
{
	char buf[80];
	FILE *fp;

	if(t->token == END) {
		fp = popen("ls -sCF", "r");
		while(fgets(buf, 80, fp))
			PRINT(buf);
		pclose(fp);
		return OK;
	}

	if(illegal_directory(&CmdLine[t->location]))
		return ERROR;

	sprintf(buf, "ls -sCF %s", &CmdLine[t->location]);
	fp = popen(buf, "r");
	while(fgets(buf, 80, fp))
		PRINT(buf);
	pclose(fp);
	return OK;
}

int
file_approve(struct TOKEN *t)
{
	char newname[80], filename[80];
	char *p;

	p = &CmdLine[t->location];
	strcpy(newname, get_string(&p));
	case_string(newname, AllUpperCase);

	strcpy(filename, newname);

	p = (char*)rindex(filename, '/');
	if(p == NULL)
		p = filename;
	case_string(p, AllLowerCase);

	if(rename(filename, newname) != OK) {
		PRINTF("File rename has failed for the two files\n");
		PRINTF("    %s\n", filename);
		PRINTF("    %s\n", newname);
		return ERROR;
	}
	return OK;
}

int
information(void)
{
	struct TOKEN *t = TokenList;
	FILE *fp;
	char *p, buf[80];

	NEXT(t);
	if(t->token == END)
		p = "DIRECTORY";
	else {
		case_string(&CmdLine[t->location], AllUpperCase);
		p = &CmdLine[t->location];
	}

	while(TRUE) {
		char *q = get_string(&p);
		if(q == NULL)
			break;

		sprintf(buf, "%s/%s", Bbs_Info_Path, q);
		init_more();
		if((fp = fopen(buf, "r")) == NULL)
			system_msg_string(500, buf);
		else {
			while(fgets(buf, 80, fp)) {
				PRINT(buf);
				if(more())
					break;
			}
			fclose(fp);
		}
	}
	return OK;
}

int
file_write_t(struct TOKEN *t)
{
	char filename[80];
	FILE *fp;
	int mnum = 0;
	char *p;

	filename[0] = 0;

	while(t->token != END) {
		switch(t->token) {
		case NUMBER:
			mnum = t->value;
			NEXT(t);
			break;

		case STRING:
			if(filename[0]) {
				bad_cmd(-1, t->location);
				NEXT(t);
				continue;
			}
			strcpy(filename, t->lexem);
			NEXT(t);
			break;

		case WORD:
			if(filename[0]) {
				bad_cmd(-1, t->location);
				NEXT(t);
				continue;
   		}
			p = &CmdLine[t->location];
			strcpy(filename, get_string(&p));
			t = resync_token_via_location(t, p);
			break;

		default:
			bad_cmd(-1, t->location);
			return ERROR;
		}
	}

	case_string(filename, AllUpperCase);

	if(illegal_directory(&CmdLine[t->location]))
		return ERROR;

	p = (char*)rindex(filename, '/');
	if(p == NULL)
		p = filename;

	if(!ImSysop)
		case_string(p, AllLowerCase);

	if(mnum)
		filesys_write_msg(mnum, filename);
	else {
		int result;
		char buf[1024];

		if((fp = fopen(filename, "w")) == NULL) {
			PRINTF("Failed to open file %s, notify the SYSOP\n", filename);
			return ERROR;
		}
		result = get_file_body(fp);
		fclose(fp);

		sprintf(buf, "Location: %s/%s", CurDir, filename);
		buffer_msg_to_user(Bbs_Sysop, buf);
		sprintf(buf, "File written by %s", usercall);
		send_msg_to_user(Bbs_Sysop, buf, buf);

		if(result == ERROR)
			unlink(filename);
	}
	return OK;
}


int
file_read_t(struct TOKEN *t)
{
	FILE *fp;
	char *p, buf[80];

	case_string(&CmdLine[t->location], AllUpperCase);
	p = &CmdLine[t->location];

	while(TRUE) {
		char *q = get_string(&p);
		if(q == NULL)
			break;

		if(illegal_directory(&CmdLine[t->location]))
			return ERROR;

		init_more();
		if((fp = fopen(q, "r")) == NULL) {
			convert_dash2underscore(q);
			if((fp = fopen(q, "r")) == NULL) {
				char *s = (char*)rindex(q, '/');
				if(s == NULL)
					s = q;
				case_string(s, AllLowerCase);
				if((fp = fopen(q, "r")) != NULL) {
					if(!ImSysop) {
						fclose(fp);
						fp = NULL;
					}
					PRINT("This file is not yet approved. The file will\n");
					PRINT("be available when it's name is all capitals\n");
				} else
					system_msg_string(500, q);
			}
		}

		if(fp) {
			while(fgets(buf, 80, fp)) {
				PRINT(buf);
				if(more())
					break;
			}
			fclose(fp);
		}
	}
	return OK;
}

/*ARGSUSED*/
int
illegal_directory(char *str)
{
	return OK;
}

int
get_file_body(FILE *fp)
{
	struct text_line *tl = NULL, *t;

	system_msg(997);
	
	if(get_body(&tl, FILE_BODY, NULL, NULL, NULL) != mTERM)
		return ERROR;
	t = tl;
	while(t) {
		fprintf(fp, "%s\n", t->s);
		NEXT(t);
	}
	textline_free(tl);

	return OK;
}

#ifdef NOMSGD
void
filesys_server(int msg_num, char *path)
{
	char msgname[80];
	char filename[80];

	sprintf(msgname, "%s/%05d", MSGBODYPATH, msg_num);
	sprintf(filename, "%s/%s", Bbs_FileSys_Path, path);

	link(msgname, filename);
}
#endif

int
filesys_write_msg(int msgnum, char *path)
{
	FILE *fpw;
	char buf[4096];
	char *result = "File written successfully";
	struct msg_dir_entry msg;
	struct text_line *tl;

	bzero(&msg, sizeof(msg));
	msg.number = msgnum;

	if (msg_ReadBodyBy(&msg, "BBS") != OK) {
		PRINTF("No message by the number %d exists.\n", msgnum);
		return ERROR;
	}

	sprintf(buf, "filesys_write_msg(msgnum=%d, path=%s)", msgnum, path);
	buffer_msg_to_user(Bbs_Sysop, buf);

	if((fpw = fopen(path, "w")) == NULL) {
		sprintf(buf, "Failed opening file for write \"%s\"", path);
		send_msg_to_user(Bbs_Sysop, "Stat: WRITE", buf);
		msg_free_body(&msg);
		return ERROR;
	}

	for (tl = msg.body; tl != NULL; NEXT(tl)) {
		/* strip routing information */
		if(!strncmp(tl->s, "R:", 2))
			continue;

		if(!strcmp(tl->s, "/EX\n"))
			break;

			/* write meat to the file */
		if(fprintf(fpw, "%s\n", tl->s) < 0) {
			result = "Error on Writing";
			break;
		}
	}

	msg_free_body(&msg);


		/* now write an audit trail at the bottom, just in case we have
		 * to know where this came from.
		 */

	fprintf(fpw,
	"\n==AuditTrail=====================================================\n");

	msg_title_write(msgnum, fpw);
	fclose(fpw);
	
	send_msg_to_user(Bbs_Sysop, "Stat: WRITE", result);
	return OK;
}

