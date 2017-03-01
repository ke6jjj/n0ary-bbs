#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "gate.h"

int operation = FALSE;
extern int shutdown_daemon;

static char *
disp_seen(int token, char *s)
{
	struct gate_entry *g;
	time_t t = 0;
	char buf[80];

	strcpy(buf, get_string(&s));

	switch(token) {
	case gUSER:
		g = GateList;
		uppercase(buf);
		while(g) {
			if(!strcmp(buf, g->call)) {
				t = g->seen;
				while(g) {
					if(g->seen > t)
						t = g->seen;
					g = g->chain;
				}
				sprintf(output, "%s\n", time2date(t));
				return output;
			}
			NEXT(g);
		}
		return Error("user not found");
	case gADDRESS:
		g = GateHash[hash_key(buf)];
		while(g) {
			if(!stricmp(buf, g->addr)) {
				sprintf(output, "%s\n", time2date(g->seen));
				return output;
			}
			g = g->hash;
		}
		return Error("address not found");
	}
	return Error("invalid command");
}

static char *
touch(char *s)
{
	char addr[80];
	struct gate_entry *g;
	if(*s == 0)
		return Error("expected address");

	strcpy(addr, get_string(&s));
	g = GateHash[hash_key(addr)];

	while(g) {
		if(!stricmp(addr, g->addr)) {
			g->seen = Time(NULL);
			g->warn_sent = 0;
			image = DIRTY;
			return JustOk;
		}
		g = g->hash;
	}
	return Error("address not found");
}

static char *
disp_user(int token, char *s)
{
	char buf[80];
	struct gate_entry *g;

	if(*s == 0) {
		switch(token) {
		case gUSER:
			return Error("expected call");
		case gADDRESS:
			return Error("expected address");
		}
		return Error("disp_user() shouldn't have gotten here");
	}

	strcpy(buf, get_string(&s));
	if(!stricmp(buf, "SEEN"))
		return disp_seen(token, s);

	switch(token) {
	case gUSER:
		g = GateList;
		output[0] = 0;
		while(g) {
			if(!stricmp(buf, g->call)) {
				while(g) {
					sprintf(output, "%s%s\n",
						output, g->addr);
					g = g->chain;
				}
				strcat(output, ".\n");
				return output;
			}
			NEXT(g);
		}
		return Error("user not found");
				
	case gADDRESS:
		g = GateHash[hash_key(buf)];
		while(g) {
			if(!stricmp(buf, g->addr)) {
				sprintf(output, "%s\n", g->call);
				return output;
			}
			g = g->hash;
		}
		return Error("address not found");
	}

	return Error("invalid command");
}

int
add_entry(char *call, char *addr)
{
	struct gate_entry *g = malloc_struct(gate_entry);
	strcpy(g->call, call);
	strcpy(g->addr, addr);
	g->seen = Time(NULL);
	if(add_user(g) == OK) {
		add_address(g);
		image = DIRTY;
		return OK;
	}
	free(g);
	return ERROR;
}

int
delete_entry(char *call, char *addr)
{
	struct gate_entry *l = NULL;
	struct gate_entry *g = GateHash[hash_key(addr)];

	while(g) {
		if(!stricmp(addr, g->addr))
			if(!stricmp(call, g->call)) {
				if(l == NULL)
					GateHash[hash_key(addr)] = g->hash;
				else
					l->hash = g->hash;
				break;
			}
		l = g;
		g = g->hash;
	}

	if(g == NULL)
		return ERROR;

	g = GateList;
	l = NULL;

	while(g) {
		if(!strcmp(call, g->call)) {
			if(!stricmp(addr, g->addr)) {
				if(g->chain != NULL) {
					if(l == NULL)
						GateList = g->chain;
					else
						l->next = g->chain;
					g->chain->next = g->next;
				} else {
					if(l == NULL)
						GateList = g->next;
					else
						l->next = g->next;
				}
				free(g);
				image = DIRTY;
				return OK;
			}

			l = g;
			g = g->chain;

			while(g) {
				if(!stricmp(addr, g->addr)) {
					l->chain = g->chain;
					free(g);
					image = DIRTY;
					return OK;
				}
				l = g;
				g = g->chain;
			}

			return ERROR;
		}
		l = g;
		NEXT(g);
	}
	return OK;
}

static char *
alter(int token, char *s)
{
	char call[80], addr[80];

	if(*s == 0)
		return Error("expected callsign");
	strcpy(call, get_string(&s));
	uppercase(call);

	if(*s == 0)
		return Error("expected address");
	strcpy(addr, get_string(&s));

	switch(token) {
	case gADD:
		if(add_entry(call, addr) == OK)
			return JustOk;
		return Error("address already in database");

	case gDELETE:
		if(delete_entry(call, addr) == OK)
			return JustOk;
		return Error("call/address pair not found");
	}
	return Error("invalid token to alter()");
}

char *
set_parameters(struct active_processes *ap, char *s)
{
	char opt[40];
	
	strcpy(opt, get_string(&s));
	uppercase(opt);

	if(!strcmp(opt, "WARN")) {
		Gated_Age_Warn = get_number(&s) * tMonth;
		return "OK\n";
	}
	if(!strcmp(opt, "KILL")) {
		Gated_Age_Kill = get_number(&s) * tMonth;
		return "OK\n";
	}
	if(!strcmp(opt, "REHASH")) {
		read_new_file();
		return "OK\n";
	}
	if(!strcmp(opt, "GATEFILE")) {
		char *val = get_string(&s);
		Gated_File = (char *)malloc(strlen(val)+1);
		strcpy(Gated_File, val);
		sprintf(output, "OK, GATEFILE = %s\n", Gated_File);
		return output;
	}

	if(!strcmp(opt, "CHECK")) {
		check_users(ap, FALSE);
		return ".\n";
	}
	if(!strcmp(opt, "CREATE")) {
		check_users(ap, TRUE);
		return ".\n";
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

static char *
help_disp(void)
{
	output[0] = 0;
	strcat(output, "USER call  ............. show email address for user\n");
	strcat(output, "ADDRESS addr  .......... show users call for address\n");
	strcat(output, "USER SEEN call  ........ when was the last time user seen\n");
	strcat(output, "ADDRESS SEEN addr  ..... last time this address was seen\n");
	strcat(output, "ADD call addr  ......... add user\n");
	strcat(output, "DELETE call addr  ...... delete user\n");
	strcat(output, "TOUCH addr  ............ touch last seen for address\n");
	strcat(output, "GUESS addr  ............ show possible matches to addr\n");
	strcat(output, "WRITE .................. write memory image to disk\n");
	strcat(output, "STAT ................... show status of daemon\n");
	strcat(output, "\n");
	strcat(output, "AGE .................... start aging\n");
	strcat(output, "DEBUG GATEFILE <fn> .... set new gateway file\n");
	strcat(output, "DEBUG REHASH  .......... init new gateway file\n");
	strcat(output, "DEBUG TIME <time_t> .... set internal clock\n");
	strcat(output, "DEBUG AGE <months> ..... set age\n");
	strcat(output, "DEBUG CHECK ............ check users\n");
	strcat(output, "DEBUG CREATE ........... create bbs entry for users\n");
	strcat(output, "DEBUG LOG [ON|OFF|CLR] . logging options\n");
	strcat(output, "EXIT  .................. disconnect from daemon\n");
	strcat(output, "SHUTDOWN  .............. terminate daemon\n");
	strcat(output, ".\n");
	return output;
}

char *
parse(struct active_processes *ap, char *s)
{
	char buf[80], cmd[80];

	struct GatedCommands *gc = GatedCmds;
	strcpy(buf, get_string(&s));

	if(buf[0] == '?')
		return help_disp();

	strcpy(cmd, buf);
	uppercase(cmd);

	if(!strcmp(cmd, "EXIT"))
		return NULL;

	if(!strcmp(cmd, "SHUTDOWN")) {
		shutdown_daemon = TRUE;
		return NULL;
	}

	if(!strcmp(cmd, "DEBUG"))
		return set_parameters(ap, s);

	if(!strcmp(cmd, "AGE")) {
		age(ap);
		return "OK\n";
	}

	while(gc->token != 0) {
		if(!stricmp(gc->key, buf)) {
			switch(gc->token) {
			case gUSER:
			case gADDRESS:
				return disp_user(gc->token, s);
			case gADD:
			case gDELETE:
				return alter(gc->token, s);
			case gTOUCH:
				return touch(s);
			case gGUESS:
				return disp_guess(s);
			case gSTAT:
				return disp_stat();
			case gWRITE:
				write_file();
				image = CLEAN;
				return JustOk;
			}
		}
		gc++;
	}
	return Error("illegal command");
}

