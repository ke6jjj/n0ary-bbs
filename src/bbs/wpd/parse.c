#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"
#include "wp.h"

int operation = FALSE;
extern int shutdown_daemon;

static char *
wpd_status(void)
{
	static char buf[1024];
	snprintf(buf, sizeof(buf), "%s%s", hash_bbs_cnt(), hash_user_cnt());
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
		return Error("Not implemented!");
	}

	if(ap->wpu == NULL)
		return Error("must set call first");

	switch(token) {
	case wUPDATE:
		user_image = DIRTY;
		ap->wpu->last_update_sent = Time(NULL);
		return "OK\n";
	case wCALL:
		snprintf(output, sizeof(output), "%s\n", ap->wpu->call);
		return output;
	case wADDRESS:
		if(ap->wpb == NULL)
			return Error("home bbs is invalid");
		snprintf(output, sizeof(output), "%s.%s\n", ap->wpb->call,
			ap->wpb->hloc);
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
		snprintf(output, sizeof(output),
			"NO, user %s not in local white pages database\n",
			call);
		return output;
	}

	ap->wpb = hash_get_bbs(ap->wpu->home);

	if(*s == 0) {
		if(ap->wpu->home[0])
			snprintf(output, sizeof(output),
				"%s @ %s.%s %s %s %s\n", ap->wpu->call,
				ap->wpu->home,
				ap->wpb ? ap->wpb->hloc : "[unknown]",
				ap->wpu->fname,
				ap->wpu->zip, ap->wpu->qth);
		else
			snprintf(output, sizeof(output),
				"%s @ [no home] %s %s %s\n", ap->wpu->call,
				ap->wpu->fname, ap->wpu->zip, ap->wpu->qth);
		return output;
	}

	return parse(ap, s);
}

static char *
help_disp(void)
{
	strlcpy(output,
		"<call>  ....................  show user entry\n"
		"CALL  ......................  show last current call\n"
		"[call] ADDRESS  ............  show home bbs and hloc\n"
		"[call] SHOW  ...............  show home bbs and hloc\n"
		"[call] SEEN [time_t]  ......  set user last seen date\n"
		"[call] CHANGED  ............  full dump of user\n"
		"[call] FNAME  ..............  user's first name\n"
		"[call] ZIP  ................  user's zip code\n"
		"[call] QTH  ................  user's city, state\n"
		"[call] HOME  ...............  user's home bbs\n"
		"[call] HLOC  ...............  user's home bbs's hloc\n"
		"[call] UPDATE  .............  flag user as updated\n"
		"    if [call] is omitted the last call will be used\n"
		"\n"
		"AGE ........................  run outgoing update cycle\n"
		"CREATE <call>  .............  create user\n"
		"KILL <call>  ...............  remove user\n"
		"BBS CREATE <call>  .........  create bbs entry\n"
		"STAT  ......................  show wp stats\n"
		"WRITE  .....................  write wp databases to disk\n"
		"UPLOAD <wp entry>  .........  feed wp update\n"
		"\n"
		"Editing\n"
		"  <call> <level> <field> <value>\n"
		"\n"
		"     level = GUESS  (lowest level)\n"
		"             USER   (trusted, assumed good)\n"
		"             SYSOP  (highest, cannot be chgd by bbs)\n"
		"\n"
		"     field = FNAME  (first name)\n"
		"             QTH    (city, st)\n"
		"             ZIP    (zip code)\n"
		"             HOME   (homebbs)\n"
		"             HLOC   (bbs hloc, only valid for bbss)\n"
		"\n"
		"SEARCH <field> <regexp>\n"
		"\n"
		"     XXX - This is not yet implemented!\n"
		"     field = CALL   (user call)\n"
		"             FNAME  (first name)\n"
		"             QTH    (city, st)\n"
		"             ZIP    (zip code)\n"
		"             HOME   (homebbs)\n"
		"\n"
		"BBS SEARCH <field> <regexp>\n"
		"\n"
		"     XXX - This is not yet implemented!\n"
		"     field = CALL   (bbs call)\n"
		"             HLOC   (bbs hloc)\n\n"
		"DEBUG TIME <time_t>  .......  set clock to fixed time\n"
		"                              (set to 0 to resume)\n"
		"DEBUG REFRESH <days>  ......  set refresh\n"
		"DEBUG AGE <months>  ........  set aging\n"
		"DEBUG REHASH  ..............  reread databases\n"
		"DEBUG USERFILE <file> ......  spec new user databases\n"
		"DEBUG BBSFILE <file> .......  spec new bbs databases\n"
		"DEBUG LOG [ON|OFF|CLR]  ....  logging options\n"
		".\n", sizeof(output));

	return output;
}

char *
set_parameters(char *s)
{
	char opt[40];
	char *newval;
	
	strlcpy(opt, get_string(&s), sizeof(opt));
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
		if (val == NULL)
			return Error("Nothing provided");
		newval = strdup(val);
		if (newval == NULL)
			return Error("Out of memory");
		if (Wpd_User_File != NULL)
			free(Wpd_User_File);
		Wpd_User_File = newval;
		snprintf(output, sizeof(output),
			"OK, USERFILE = %s\n", Wpd_User_File);
		return output;
	}
	if(!strcmp(opt, "BBSFILE")) {
		char *val = get_string(&s);
		if (val == NULL)
			return Error("Nothing provided");
		newval = strdup(val);
		if (newval == NULL)
			return Error("Out of memory");
		if (Wpd_Bbs_File != NULL)
			free(Wpd_Bbs_File);
		Wpd_Bbs_File = newval;
		snprintf(output, sizeof(output),
			"OK, BBSFILE = %s\n", Wpd_Bbs_File);
		return output;
	}

	if(!strcmp(opt, "LOG")) {
		if(*s && *s != 0) {
			strlcpy(opt, get_string(&s), sizeof(opt));
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
		return Error("unknown logging status");
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
	strlcpy(buf, get_string(&s), sizeof(buf));
	uppercase(buf);

	if(buf[0] == '?')
		return help_disp();

	if(!strcmp(buf, "EXIT"))
		return NULL;

	if(!strcmp(buf, "DEBUG"))
		return set_parameters(s);

	if(!strcmp(buf, "AGE")) {
		generate_wp_update(ap);
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

