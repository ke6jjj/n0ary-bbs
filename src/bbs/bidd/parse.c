#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"

extern int shutdown_daemon;
extern char output[4096];

static char *
bid_status(void)
{
	static char buf[1024];
	sprintf(buf, "%s", hash_cnt());
	return buf;
}

char *
set_parameters(char *s)
{
	char opt[40];
	
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
	return Error("Invalid option");
}

static char *
help_disp(void)
{
	output[0] = 0;
	strcat(output, "<bid> .................. is message a dup\n");
	strcat(output, "MID <bid> .............. is message maybe a dup\n");
	strcat(output, "ADD <bid> .............. add bid to list\n");
	strcat(output, "DELETE <bid> ........... delete a bid from list\n");
	strcat(output, "\n");
	strcat(output, "STAT ................... show status of daemon\n");
	strcat(output, "WRITE .................. write bid list to disk\n");
	strcat(output, "\n");
	strcat(output, "EXIT ................... leave daemon\n");
	strcat(output, "SHUTDOWN ............... kill daemon\n");
	strcat(output, "\n");
	strcat(output, "AGE .................... start aging\n");
	strcat(output, "DEBUG BIDFILE <fn> ..... set new bid file\n");
	strcat(output, "DEBUG REHASH  .......... init new bid file\n");
	strcat(output, "DEBUG TIME <time_t> .... set internal clock\n");
	strcat(output, "DEBUG AGE <months> ..... set bid age\n");
	strcat(output, "DEBUG LOG [ON|OFF|CLR] . logging options\n");
	strcat(output, ".\n");
	return output;
}

char *
parse(char *s)
{
	char buf[80];

	if(*s == 0)
		return "OK\n";

	strcpy(buf, get_string(&s));
	uppercase(buf);

	if(buf[0] == '?')
		return help_disp();

	if(!strcmp(buf, "DEBUG"))
		return set_parameters(s);

	if(!strcmp(buf, "AGE")) {
		age();
		if(bid_image == DIRTY)
			write_file();
		return "OK\n";
	}

	if(!strcmp(buf, "EXIT"))
		return NULL;

	if(!strcmp(buf, "SHUTDOWN")) {
		shutdown_daemon = TRUE;
		return NULL;
	}
		
	if(!strcmp(buf, "WRITE"))
		return write_file();

	if(!strcmp(buf, "DELETE"))
		return delete_bid(s);

	if(!strcmp(buf, "STAT"))
		return bid_status();

	if(!strcmp(buf, "ADD")) {
		if(*s == 0)
			return "NO, expected bid\n";
		strcpy(buf, get_string(&s));
		uppercase(buf);
		return add_bid(buf);
	}

	if(!strcmp(buf, "MID")) {
		strcpy(buf, get_string(&s));
		uppercase(buf);
		return chk_mid(buf);
	}

	return chk_bid(buf);
}
