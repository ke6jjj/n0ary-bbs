#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tcpd.h"

extern int shutdown_daemon;
extern char output[4096];


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

	return Error("Invalid option");
}

static char *
help_disp(void)
{
	output[0] = 0;

	strcat(output, "BBS call ............... login to bbs\n");
	strcat(output, "\n");
	strcat(output, "MSGD ................... connect to message daemon\n");
	strcat(output, "WPD .................... connect to wp daemon\n");
	strcat(output, "BIDD ................... connect to bid daemon\n");
	strcat(output, "USERD .................. connect to user daemon\n");
	strcat(output, "GATED .................. connect to gateway daemon\n");
	strcat(output, "BBSD ................... connect to bbs control daemon\n");
	strcat(output, "TCPD host .............. connect to remote tcp daemon\n");
	strcat(output, "TELNET ................. connect telnet session\n");
	strcat(output, "\n");
	strcat(output, "EXIT ................... leave daemon\n");
	strcat(output, "SHUTDOWN ............... kill daemon\n");
	strcat(output, "DEBUG LOG [ON|OFF|CLR] . logging options\n");
	strcat(output, ".\n");
	return output;
}

static char *
connect_to(struct active_processes *ap, char *host, int port)
{
	if((ap->sock = socket_open(host, port)) == ERROR)
		return Error("Could not connect to requested daemon");
	return "Connecting...\n";
}

static char *
chain_bbs(struct active_processes *ap, char *call)
{
	int port = 0;

	if((ap->listen = socket_listen(NULL, &port)) == ERROR)
		return Error("Couldn't open listen socket for bbs");

	uppercase(call);

	if(fork() == 0) {
		char cmd[256];
		char addr[LenREMOTEADDR];
		struct RemoteAddr raddr;
		char p[10];
		snprintf(addr, sizeof(addr), "unknown:");
		get_remote_addr(ap->fd, &raddr);
		print_remote_addr(&raddr, addr, sizeof(addr));
		sprintf(p, "%d", port);
		sprintf(cmd, "%s/b_bbs", Bin_Dir);
		execl(cmd, "b_bbs", "-u", "-s", p, "-v", "TCP", "-a",
			addr, call, 0);
		exit(1);
	}
	return Ok("Waiting for connection");
}

char *
spawn_program(struct active_processes *ap, char *prog)
{
	int port = 0;

	if((ap->listen = socket_listen(NULL, &port)) == ERROR)
		return Error("Couldn't open listen socket for program");

	if(fork() == 0) {
		char cmd[256];
		int sock;
		
		sleep(2);
		if((sock = socket_open(Bbs_Host, port)) == ERROR) {
			printf("Failure to open socket to tcpd\n");
			exit(1);
		}
		close(0);
		close(1);
		close(2);
		dup(sock);
		dup(sock);
		dup(sock);
		system("/bin/sh");
#if 0
		execl("/bin/sh", "sh", 0);
		execl("/usr/ucb/rsh", "rsh", Bbs_Host, "-l", "bbs_cntrl", 0);
#endif
		exit(1);
	}

	return Ok("Waiting for connection");
}

void
spawn_bbs(int fd, char *call, int sysop)
{
	int var = fcntl(fd, F_GETFL, FNDELAY);
	var &= 0xFFFB;
	fcntl(fd, F_SETFL, var);

	uppercase(call);

	if(fork() == 0) {
		char cmd[256];
		char addr[LenREMOTEADDR];
		struct RemoteAddr raddr;
		snprintf(addr, sizeof(addr), "unknown:");
		get_remote_addr(fd, &raddr);
		print_remote_addr(&raddr, addr, sizeof(addr));

		close(0);
		close(1);
		close(2);
		dup(fd);
		dup(fd);
		dup(fd);
		sprintf(cmd, "%s/b_bbs", Bin_Dir);
		if(sysop)
			execl(cmd, "b_bbs", "-u", "-v", "TCP", "-a", addr,
				call, 0);
		else
			execl(cmd, "b_bbs", "-v", "TCP", "-a", addr, call, 0);
		exit(1);
	}
}

char *
parse(struct active_processes *ap, char *s)
{
	char buf[80];

	if(*s == 0)
		return "OK\n";

	strcpy(ap->cmd, s);

	strcpy(buf, get_string(&s));
	uppercase(buf);


	if(buf[0] == '?')
		return help_disp();

	if(!strcmp(buf, "DEBUG"))
		return set_parameters(s);

	if(!strcmp(buf, "EXIT"))
		return NULL;

	if(!strcmp(buf, "SHUTDOWN")) {
		shutdown_daemon = TRUE;
		return NULL;
	}
	
	if(!strncmp(buf, "STAT", 4))
		return show_status(ap);

	if(!strcmp(buf, "WPD"))
		return connect_to(ap, Bbs_Host, Wpd_Port);

	if(!strcmp(buf, "MSGD"))
		return connect_to(ap, Bbs_Host, Msgd_Port);

	if(!strcmp(buf, "GATED"))
		return connect_to(ap, Bbs_Host, Gated_Port);

	if(!strcmp(buf, "USERD"))
		return connect_to(ap, Bbs_Host, Userd_Port);

	if(!strcmp(buf, "BIDD"))
		return connect_to(ap, Bbs_Host, Bidd_Port);

	if(!strcmp(buf, "BBSD"))
		return connect_to(ap, Bbs_Host, Bbsd_Port);

	if(!strcmp(buf, "TCPD"))
		return connect_to(ap, get_string(&s), Tcpd_Port);

	if(!strcmp(buf, "TEST")) {
		return spawn_program(ap, get_string(&s));
	}

	if(!strcmp(buf, "BBS")) {
		if(*s == 0)
			return Error("Expected a call");
		spawn_bbs(ap->fd, get_string(&s), FALSE);
		return NULL;
	}
	if(!strcmp(buf, "SBBS")) {
		if(*s == 0)
			return Error("Expected a call");
		return chain_bbs(ap, get_string(&s));
	}
		
	return Error("Invalid command");
}

