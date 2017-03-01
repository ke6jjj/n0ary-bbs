#include <stdio.h>

#include "c_cmmn.h"
#include "tools.h"

void
	uutry(char*), lock_bbs(char*),
	shutdown_system(char*), drop_phone(char*), respawn(char*),
	hello(), system_status(char*), report_ups(char*);

struct commands {
	char *code;
	int len;
	void (*func)();
} cmds[30] = {
	{ "0",  1,  system_status },
	{ "1",  1,  report_ups },
	{ "2",	1,	respawn },
	{ "3",	1,	uutry },
	{ "4",	1,	lock_bbs },
	{ "88", 2,	drop_phone },
	{ "99", 2,	shutdown_system },
	{ 0, 0, 0 }
};

main(int argc, char *argv[])
{
	int i;
	char *str = argv[1];

	if(*str == 0)
		hello();
	else {
		i = 0;
		while(cmds[i].len) {
			if(!strncmp(str, cmds[i].code, cmds[i].len)) {
				cmds[i].func(&(str[cmds[i].len]));
				break;
			}
			i++;
		}
	}
}

void
hello(char *s)
{
	talk("system status port");
}

void
sys(char *cmd)
{
	char buf[80];
	FILE *fp = popen(cmd, "r");
	if(fp == NULL)
		return;

	fgets(buf, 80, fp);
	pclose(fp);
	talk(buf);
}

void
system_status(char *s)
{
	sys("/usr/etc/ping araengr");
	sys("/usr/etc/ping suny");
	sys("/usr/etc/ping violet");
	sys("/usr/etc/ping arrow");
	sys("/usr/etc/ping thumper");
}

void
report_ups(char *s)
{
	int bv, iv, ov;
	char buf[256];

	talk("Emergency power status");

	switch(read_sola(&bv, &iv, &ov)) {
	case TRUE:
		talk("On line");
		break;
	case FALSE:
		talk("On battery");
		break;
	case ERROR:
		talk("oops, we have a problem with the sola daemon");
		return;
	}

	sprintf(buf, "input is %d volts, output is %d volts, battery is %d volts",
		iv, ov, bv);
	talk(buf);
}

char *
which_host(char *s)
{
	if(!strcmp(s, "192*135*76*1"))
		return "araengr";
	if(!strcmp(s, "192*135*76*2"))
		return "suny";
	if(!strcmp(s, "192*135*76*3"))
		return "arrow";
	if(!strcmp(s, "192*135*76*5"))
		return "violet";

	return NULL;
}

void
uutry(char *s)
{
	char *host, buf[80];

	switch(*s) {
	case '0':
		host = "baycom";
		break;
	case '1':
		host = "video";
		break;
	default:
		return;
	}

	sprintf(buf, "Initiating U U C P connection to %s", host);
	talk(buf);

	sprintf(buf, "/bbs/bin/Uutry %s", host);
	system(buf);
}

void
shutdown_system(char *s)
{
	char *cmd = s++;
	char *host;
	char buf[80];

	if((host = which_host(s)) == NULL)
		return;

	switch(*cmd) {
	case '7':
		cmd = "/usr/etc/halt";
		sprintf(buf, "halting %s", host);
		break;
	case '8':
		cmd = "/usr/etc/reboot";
		sprintf(buf, "rebooting %s", host);
		break;
	case '9':
		cmd = "/usr/etc/shutdown -k +1";
		sprintf(buf, "shutting down %s in one minute", host);
		break;
	default:
		return;
	}

	if(fork() == 0) {
		execl("/usr/ucb/rsh", "rsh", host, cmd, 0);
		exit(1);
	}
	talk(buf);
}

void
drop_phone(char *s)
{
	char *port;
	char *voice;

	if(!strcmp(s, "0605")) {
		port = "s0";
		voice = "Hanging up S 0";
	} else
	if(!strcmp(s, "1950")) {
		port = "s1";
		voice = "Hanging up S 1";
	} else
	if(!strcmp(s, "9788")) {
		port = "s4";
		voice = "Hanging up S 4";
	} else
	if(!strcmp(s, "0501")) {
		port = "s3";
		voice = "Hanging up S 3";
	} else
		return;

	if(fork() == 0) {
		execl("/usr/tools/pm/2.2/pmreset", "pmreset", "arrow", "luxman", port, 0);
		exit(1);
	}
	talk(voice);
}

void
respawn(char *s)
{
	int echo = FALSE;
	char *proc = NULL;
	char *name;

	while(*s) {
		switch(*s++) {
		case '0':
			echo = TRUE;
#if 0
			talk("Spawn Menu");
			talk("1 tnc 144");
			talk("2 tnc 220");
			talk("3 tnc 440");
			talk("4 dtmf");
			talk("5 sola");
			talk("6 dectalk");
#endif
			continue;
		case '1':
			proc = "/bbs/bin/respawn \"/bbs/bin/ntncd 144\"";
			name = "spawn tnc 144";
			break;
		case '2':
			proc = "/bbs/bin/respawn \"/bbs/bin/ntncd 220\"";
			name = "spawn tnc 220";
			break;
		case '3':
			proc = "/bbs/bin/respawn \"/bbs/bin/ntncd 440\"";
			name = "spawn tnc 440";
			break;
		case '4':
			proc = "/bbs/bin/respawn /bbs/bin/dtmf";
			name = "spawn dtmf";
			break;
		case '5':
			proc = "/bbs/bin/respawn /bbs/bin/sola";
			name = "spawn sola";
			break;
		case '6':
			proc = "/bbs/bin/respawn /bbs/bin/dectalk";
			name = "spawn dectalk";
			break;
		}

		if(proc) {
			talk(name);
			if(echo == FALSE)
				system(proc);
		}
	}
}

void
lock_bbs(char *s)
{
	char c = *s++;
	char *port;

#if 0
	switch(c) {
	case '0':
		talk("Clear locks on");
		break;
	case '1':
		talk("Set locks on");
		break;
	default:
		return;
	}

	bbsd_check_in("REMOTE", L_STATUS);

	while(*s) {
		int via = *s++ - '0';

		switch(via) {
		case L_TNC144:
			port = "2 meter"; break;
		case L_TNC220:
			port = "2 20 "; break;
		case L_TNC440:
			port = "4 40"; break;
		case L_PHONE1:
			port = "phone 1 9 5 0"; break;
		case L_PHONE2:
			port = "phone 0 6 0 5"; break;
		case L_CONSOLE:
			port = "console"; break;
		case L_SERVER:
			port = "server"; break;
		case L_CRON:
			port = "cron"; break;
		case L_SMTP:
			port = "s m t p"; break;
		}

		talk(port);

		if(c == '0')
			clr_lock(via);
		else
			set_lock(via, "Maintenance in progress, please try later");
	}

	bbsd_check_out();
#endif
}
