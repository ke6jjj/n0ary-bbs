#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

int operation = FALSE;

extern int shutdown_daemon;

static char *
help(void)
{
	return "OK\n";
}

char *
parse_cmd(struct active_processes *ap, int token, char *s)
{
	char *result;
	int operation_save;

	switch(token) {
	case uSTATUS:
		return usrfile_show();

	case uFLUSH:
		usrfile_close(ap);
		return "OK\n";

	case uUSER:
		return disp_user(s);
	case uAGE:
		return age_users(ap);
	case uCREATE:
		return create_user(ap, s);
	case uKILL:
		return kill_user(s);
	case uLOCATE:
		return find_user(s);
	case uSUFFIX:
		return find_user_by_suffix(s);
	case uSEARCH:
		return "OK\n";
	}

	if(ap->ul == NULL)
		return Error("must set call first");

	switch(token) {
	case uCLEAR:
	case uSET:
		operation_save = operation;
		operation = token;
		result = parse(ap, s);
		operation = operation_save;
		return result;

	case uCALL:
		sprintf(output, "%s\n", ap->ul->call);
		return output;
	case uLOGIN:
		return login_user(ap, s);
	case uSHOW:
		return show_user(ap);
	default:
		return Error("either illegal or unimplemented");
	}
}

char *
set_parameters(char *s)
{
	char opt[40];

	if(!s || *s == 0)
		return Error("Expected option");

	strcpy(opt, get_string(&s));
	uppercase(opt);

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
	return Error("invalid DEBUG command");
}

char *
parse(struct active_processes *ap, char *s)
{
	char *cmd = NULL;

	if(*s == '?')
		return help();

	cmd = copy_string(s);
	uppercase(cmd);

	if(!strncmp(cmd, "EXIT", 4))
		return NULL;

	if(!strncmp(cmd, "SHUTDOWN", 8)) {
		shutdown_daemon = TRUE;
		return NULL;
	}

	if(!strncmp(cmd, "DEBUG", 5)) {
		get_string(&s);  /* eat debug */
		return set_parameters(s);
	}
	free(cmd);

	while(*s) {
		struct UserdCommands *uc = UserdCmds;
		char *buf = NULL;

		buf = copy_string(get_string(&s));
		uppercase(buf);

		while(uc->token != 0) {
			if(!strcmp(uc->key, buf)) {
				free(buf);
				if(uc->type == uCmd)
					return parse_cmd(ap, uc->token, s);

				if(ap->ul == NULL)
					return Error("must set call first");

				switch(uc->type) {
				case uToggle:
					return edit_toggle(ap, uc->token, s);
				case uString:
					return edit_string(ap, uc->token, s);
				case uList:
					return edit_list(ap, uc->token, s);
				case uNumber:
					return edit_number(ap, uc->token, s);
				}
			}
			uc++;
		}

		if(usrfile_read(buf, ap) == NULL) {
			free(buf);
			return Error("not a user");
		}
		free(buf);
	}
	return "OK\n";
}
