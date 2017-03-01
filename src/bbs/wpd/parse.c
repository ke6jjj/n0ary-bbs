#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

int operation = FALSE;
extern int shutdown_daemon;

static char *
wpd_status(void)
{
	static char buf[1024];
	sprintf(buf, "%s%s", hash_bbs_cnt(), hash_user_cnt());
	return buf;
}

char *
parse_cmd(struct active_processes *ap, int token, char *s)
{
	char *result;

	switch(token) {
	case wSTAT:
		return wpd_status();

	case wWRITE:
		write_user_file();
		write_bbs_file();
		return "OK\n";

	case wBBS:
		bbs_mode = TRUE;
		result = parse(ap, s);
		bbs_mode = FALSE;
		return result;

	case wCREATE:
		return create_user(ap, s);
	case wKILL:
		return kill_user(s);
	case wUPLOAD:
		return upload(s);
	case wSEARCH:
		return "OK\n";
	}

	if(ap->wpu == NULL)
		return Error("must set call first");

	switch(token) {
	case wUPDATE:
		user_image = DIRTY;
		ap->wpu->last_update_sent = Time(NULL);
		return "OK\n";
	case wCALL:
		if(ap->wpu == NULL)
			return Error("no user currently selected");
		sprintf(output, "%s\n", ap->wpu->call);
		return output;
	case wADDRESS:
		if(ap->wpu == NULL)
			return Error("no user currently selected");
		if(ap->wpb == NULL)
			return Error("home bbs is invalid");
		sprintf(output, "%s.%s\n", ap->wpb->call, ap->wpb->hloc);
		return output;
	case wSEEN:
		{
			time_t when = (time_t)get_number(&s);
			user_image = DIRTY;
			ap->wpu->seen = (when != ERROR) ? when : Time(NULL);
			if(ap->wpu->firstseen == 0)
				ap->wpu->firstseen = ap->wpu->seen;
		}
		return "OK\n";

	case wSHOW:
		return show_user(ap);
	case wCHANGED:
		return report_update(ap);
	default:
		return Error("either illegal or unimplemented");
	}

}

char *
set_call(struct active_processes *ap, char *call, char *s)
{
	if((ap->wpu = hash_get_user(call)) == NULL) {
		sprintf(output, "NO, user %s not in local white pages database\n", call);
		return output;
	}

	ap->wpb = hash_get_bbs(ap->wpu->home);

	if(*s == 0) {
		sprintf(output, "%s @", ap->wpu->call);
		if(ap->wpu->home[0])
			sprintf(output, "%s %s.%s %s %s %s\n", output, ap->wpu->home,
				ap->wpb ? ap->wpb->hloc : "[unknown]", ap->wpu->fname,
				ap->wpu->zip, ap->wpu->qth);
		else
			sprintf(output, "%s [no home] %s %s %s\n", output, 
				ap->wpu->fname, ap->wpu->zip, ap->wpu->qth);
		return output;
	}

	return parse(ap, s);
}

static char *
help_disp(void)
{
	output[0] = 0;
	strcat(output, "<call>  ....................  show user entry\n");
	strcat(output, "CALL  ......................  show last current call\n");
	strcat(output, "[call] ADDRESS  ............  show home bbs and hloc\n");
	strcat(output, "[call] SHOW  ...............  show home bbs and hloc\n");
	strcat(output, "[call] SEEN [time_t]  ......  set user last seen date\n");
	strcat(output, "[call] CHANGED  ............  full dump of user\n");
	strcat(output, "[call] FNAME  ..............  user's first name\n");
	strcat(output, "[call] ZIP  ................  user's zip code\n");
	strcat(output, "[call] QTH  ................  user's city, state\n");
	strcat(output, "[call] HOME  ...............  user's home bbs\n");
	strcat(output, "[call] HLOC  ...............  user's home bbs's hloc\n");
	strcat(output, "[call] UPDATE  .............  flag user as updated\n");
	strcat(output, "    if [call] is omitted the last call will be used\n");
	strcat(output, "\n");

	strcat(output, "CREATE <call>  .............  create user\n");
	strcat(output, "KILL <call>  ...............  remove user\n");
	strcat(output, "BBS CREATE <call>  .........  create bbs entry\n");
	strcat(output, "STAT  ......................  show wp stats\n");
	strcat(output, "WRITE  .....................  write wp databases to disk\n");
	strcat(output, "UPLOAD <wp entry>  .........  feed wp update\n");
	strcat(output, "\n");

	strcat(output, "Editing\n");
	strcat(output, "  <call> <level> <field> <value>\n");
	strcat(output, "\n");
	strcat(output, "     level = GUESS  (lowest level)\n");
	strcat(output, "             USER   (trusted, assumed good)\n");
	strcat(output, "             SYSOP  (highest, cannot be chgd by bbs)\n");
	strcat(output, "\n");
	strcat(output, "     field = FNAME  (first name)\n");
	strcat(output, "             QTH    (city, st)\n");
	strcat(output, "             ZIP    (zip code)\n");
	strcat(output, "             HOME   (homebbs)\n");
	strcat(output, "             HLOC   (bbs hloc, only valid for bbss)\n");
	strcat(output, "\n");

	strcat(output, "SEARCH <field> <regexp>\n");
	strcat(output, "\n");
	strcat(output, "     field = CALL   (user call)\n");
	strcat(output, "             FNAME  (first name)\n");
	strcat(output, "             QTH    (city, st)\n");
	strcat(output, "             ZIP    (zip code)\n");
	strcat(output, "             HOME   (homebbs)\n");
	strcat(output, "\n");
	strcat(output, "BBS SEARCH <field> <regexp>\n");
	strcat(output, "\n");
	strcat(output, "     field = CALL   (bbs call)\n");
	strcat(output, "             HLOC   (bbs hloc)\n\n");
	strcat(output, "DEBUG TIME <time_t>  .......  set clock to fixed time\n");
	strcat(output, "DEBUG REFRESH <days>  ......  set refresh\n");
	strcat(output, "DEBUG AGE <months>  ........  set aging\n");
	strcat(output, "DEBUG REHASH  ..............  reread databases\n");
	strcat(output, "DEBUG USERFILE <file> ......  spec new user databases\n");
	strcat(output, "DEBUG BBSFILE <file> .......  spec new bbs databases\n");
	strcat(output, "DEBUG LOG [ON|OFF|CLR]  ....  logging options\n");
	strcat(output, ".\n");
	return output;
}

char *
set_parameters(char *s)
{
	char opt[40];
	
	strncpy(opt, get_string(&s), 39);
	uppercase(opt);

	if(!strcmp(opt, "REFRESH")) {
		Wpd_Refresh = get_number(&s) * tDay;
		return "OK\n";
	}
	if(!strcmp(opt, "AGE")) {
		Wpd_Age = get_number(&s) * tMonth;
		return "OK\n";
	}
	if(!strcmp(opt, "REHASH")) {
		startup();
		return "OK\n";
	}
	if(!strcmp(opt, "USERFILE")) {
		char *val = get_string(&s);
		Wpd_User_File = (char *)malloc(strlen(val)+1);
		strcpy(Wpd_User_File, val);
		sprintf(output, "OK, USERFILE = %s\n", Wpd_User_File);
		return output;
	}
	if(!strcmp(opt, "BBSFILE")) {
		char *val = get_string(&s);
		Wpd_Bbs_File = (char *)malloc(strlen(val)+1);
		strcpy(Wpd_Bbs_File, val);
		sprintf(output, "OK, BBSFILE = %s\n", Wpd_Bbs_File);
		return output;
	}

	if(!strcmp(opt, "LOG")) {
		if(*s && *s != 0) {
			strcpy(opt, get_string(&s));
			uppercase(opt);
			if(!strcmp(opt, "ON"))
				Logging = logON;
			if(!strcmp(opt, "CLR"))
				Logging = logONnCLR;
			if(!strcmp(opt, "OFF"))
				Logging = logOFF;
		}
		switch(Logging) {
		case logOFF:
			return Ok("logging is off");
		case logON:
			return Ok("logging is on");
		case logONnCLR:
			return Ok("logging is on with clearing enabled");
		}
		return Ok("unknown logging status");
	}

	if(!strcmp(opt, "TIME")) {
		time_now = str2time_t(s);
		return "OK\n";
	}

	return Error("Invalid option");
}

char *
parse(struct active_processes *ap, char *s)
{
	char buf[80];

	struct WpdCommands *wc = WpdCmds;
	strncpy(buf, get_string(&s), 79);
	uppercase(buf);

	if(buf[0] == '?')
		return help_disp();

	if(!strcmp(buf, "EXIT"))
		return NULL;

	if(!strcmp(buf, "DEBUG"))
		return set_parameters(s);

	if(!strcmp(buf, "AGE")) {
		generate_wp_update(NULL);
		return "OK\n";
	}

	if(!strcmp(buf, "SHUTDOWN")) {
		shutdown_daemon = TRUE;
		return NULL;
	}

	while(wc->token != 0) {
		if(!strcmp(wc->key, buf)) {
			if(ap->wpu == NULL) {
				if(wc->type == wCmd)
					return parse_cmd(ap, wc->token, s);
				return Error("must set call first");
			}
			switch(wc->type) {
			case wLevel:
				return set_level(ap, wc->token, s);
			case wString:
				return edit_string(ap, wc->token, s);
			case wNumber:
				return edit_number(ap, wc->token, s);
			case wCmd:
				return parse_cmd(ap, wc->token, s);
			}
		}
		wc++;
	}
	return set_call(ap, buf, s);
}

