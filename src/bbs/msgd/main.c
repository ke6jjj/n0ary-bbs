#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "msgd.h"
#include "bid.h"

struct active_processes *procs = NULL;

char output[4096];
int shutdown_daemon = FALSE;

time_t
	time_now = 0,
	Msgd_Age_Interval,
	Msgd_Age_Old,
	Msgd_Age_Active[3],
	Msgd_Age_Killed[3];

int
	Msgd_Port;

char 
	*Bbs_Call,
	*Msgd_Bind_Addr = NULL,
	*Msgd_Body_Path,
	*Msgd_Archive_Path = NULL,
	*Msgd_Fwd_Dir,
	*Msgd_Route_File,
	*Msgd_Group_File,
	*Msgd_Global_Bid_File,
	*Msgd_System_File;

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "",					tCOMMENT,	NULL },
	{ "MSGD_PORT",			tINT,		(int*)&Msgd_Port },
	{ "MSGD_BIND_ADDR",		tSTRING,	(int*)&Msgd_Bind_Addr },
	{ "MSGD_BODY_PATH",		tDIRECTORY,	(int*)&Msgd_Body_Path },
	{ "MSGD_ARCHIVE_PATH",		tDIRECTORY,	(int*)&Msgd_Archive_Path },
	{ "MSGD_FWD_DIR",		tDIRECTORY,	(int*)&Msgd_Fwd_Dir },
	{ "MSGD_ROUTE_FILE",	tFILE,		(int*)&Msgd_Route_File },
	{ "MSGD_SYSTEM_FILE",	tFILE,		(int*)&Msgd_System_File },
	{ "MSGD_GROUP_FILE",	tFILE,		(int*)&Msgd_Group_File },
	{ "MSGD_GLOBAL_BID_FILE",	tFILE,	(int*)&Msgd_Global_Bid_File },
	{ "",					tCOMMENT,	NULL },
	{ "MSGD_AGE_INTERVAL",	tTIME,		(int*)&Msgd_Age_Interval },
	{ "MSGD_AGE_OLD",		tTIME,		(int*)&Msgd_Age_Old },

	{ "MSGD_AGE_P_ACTIVE",	tTIME,		(int*)&Msgd_Age_Active[PERS] },
	{ "MSGD_AGE_P_KILLED",	tTIME,		(int*)&Msgd_Age_Killed[PERS] },
	{ "MSGD_AGE_B_ACTIVE",	tTIME,		(int*)&Msgd_Age_Active[BULL] },
	{ "MSGD_AGE_B_KILLED",	tTIME,		(int*)&Msgd_Age_Killed[BULL] },
	{ "MSGD_AGE_T_ACTIVE",	tTIME,		(int*)&Msgd_Age_Active[NTS] },
	{ "MSGD_AGE_T_KILLED",	tTIME,		(int*)&Msgd_Age_Killed[NTS] },
	{ NULL, 0, NULL}};

int
service_port(struct active_processes *ap)
{
    char *c, *s, buf[256];

	if(socket_read_line(ap->fd, buf, 256, 10) == ERROR)
		return ERROR;

	log_f("msgd", "R:", buf);
	s = buf;
	NextChar(s);
	if(*s) {
		if((c = parse(ap, s)) == NULL)
			return ERROR;
	} else
		c = Error("call");

	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

	log_f("msgd", "S:", c);
	return OK;
}

struct active_processes *
add_proc()
{
    struct active_processes
        *tap = procs,
        *ap = malloc_struct(active_processes);

    if(tap == NULL)
        procs = ap;
    else {
        while(tap->next)
            NEXT(tap);
        tap->next = ap;
    }

    ap->fd = ERROR;
	ap->list_sent = FALSE;
	ap->list_mode = NormalMode;
	ap->disp_mode = dispNORMAL;
    return ap;
}

struct active_processes *
remove_proc(struct active_processes *ap)
{
    struct active_processes *tap = procs;

    if((long)procs == (long)ap) {
        NEXT(procs);
		tap = procs;
    } else {
        while(tap) {
            if((long)tap->next == (long)ap) {
                tap->next = tap->next->next;
				NEXT(tap);
                break;
            }
            NEXT(tap);
        }
    }    

    socket_close(ap->fd);

    free(ap);
    return tap;
}

void
usage(char *pgm)
{
	printf("usage: %s [-d#] [-?|h] [-w] [-f config.file]\n", pgm);
	printf("          -?/H\tdisplay help\n");
	printf("          -w\twrite configuration to stdout\n");
	printf("          -f\tread configuration file\n");
	printf("          -l\tenable logging\n");
	printf("          -L\tenable logging with idle clear\n");
	printf("          -d#\tdebugging options (hex)\n");
	printf("              1     verbose\n");
	printf("              100   leave daemon in foreground\n");
	printf("              200   ignore host\n");
	printf("              400   don't connect to other daemons\n");
	printf("\n");
}

int
main(int argc, char *argv[])
{
	int listen_port;
	char *listen_addr;
	int listen_sock;
	int bbsd_sock;
	struct timeval t;
	struct active_processes *ap;
	time_t age_time;

	parse_options(argc, argv, ConfigList, "MSGD - Message Daemon");

	if(bbsd_open(Bbs_Host, Bbsd_Port, "MSGD", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	Logging = logON;
	log_f("msgd", "******", "Coming UP");
	Logging = logOFF;

	bbsd_get_configuration(ConfigList);
	bbsd_msg("Startup");
	bbsd_sock = bbsd_socket();
	time_now = bbsd_get_time();

	age_time = Time(NULL) + Msgd_Age_Interval;
	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

	if (build_msgdir()) {
		Logging = logON;
		log_f("msgd", "Can't build_msgdir()", "");
		exit(1);
	}

	if (initialize_bids(Msgd_Global_Bid_File, get_max_message_id()) != 0) {
		Logging = logON;
		log_f("msgd", "Can't initialize_bids()", "");
		exit(1);
	}

	listen_port = Msgd_Port;
	listen_addr = Msgd_Bind_Addr;
	if((listen_sock = socket_listen(listen_addr, &listen_port)) == ERROR)
		exit(1);

	if(bbsd_port(Msgd_Port))
		error_print_exit(0);
	if(dbug_level & dbgVERBOSE)
		printf("UP\n");

	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;

		if(shutdown_daemon)
			exit(0);

		error_print();

		ap = procs;
		FD_ZERO(&ready);
		FD_SET(listen_sock, &ready);
		fdlimit = listen_sock;
		
		FD_SET(bbsd_sock, &ready);
		if(bbsd_sock > fdlimit)
			fdlimit = bbsd_sock;

		while(ap) {
			if(ap->fd != ERROR) {
				FD_SET(ap->fd, &ready);
				if(ap->fd > fdlimit)
					fdlimit = ap->fd;
			}
			NEXT(ap);
		}
		fdlimit += 1;

		t.tv_sec = 60;
		t.tv_usec = 0;

		if((cnt = select(fdlimit, (void*)&ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			time_t now = Time(NULL);
			if(now > age_time) {
				age_time = now + Msgd_Age_Interval;
				if(!(dbug_level & dbgNODAEMONS))
					bbsd_msg("Aging");
				age_messages();
				if(!(dbug_level & dbgNODAEMONS))
					bbsd_msg("");
			}
			continue;
		}

		if(FD_ISSET(bbsd_sock, &ready))
			shutdown_daemon = TRUE;

        if(FD_ISSET(listen_sock, &ready)) {
            ap = add_proc();
            if((ap->fd = socket_accept(listen_sock)) < 0)
                ap = remove_proc(ap);
			else
				if(socket_write(ap->fd, daemon_version("msgd", Bbs_Call)) == ERROR)
					ap = remove_proc(ap);
        }

        ap = procs;
        while(ap) {
            if(FD_ISSET(ap->fd, &ready)) {
                if(service_port(ap) == ERROR) {
                    ap = remove_proc(ap);
					continue;
				}
            }
            NEXT(ap);
        }

		wait3(NULL, WNOHANG, NULL);
	}

	return 0;
}

char *
msg_stats(void)
{
	struct active_processes *ap = procs;

	output[0] = 0;

	while(ap) {
		snprintf(output, sizeof(output), "%s%s\t%s\t%s\tGrp:%s\n",
			output, ap->call,
			ap->list_sent ? "SENT":"PEND",
			(ap->list_mode==NormalMode) ? "NORM" :
				(ap->list_mode==SysopMode) ? "SYSOP" :
				(ap->list_mode==BbsMode) ? "BBS" :
				(ap->list_mode==MineMode) ? "MINE" : "SINCE",
			ap->grp ? ap->grp->name :"None");
		NEXT(ap);
		if(strlen(output) > 3500) {
			strlcat(output, ".... too large ....\n",
				sizeof(output));
			break;
		}
	}

	type_stats();
	fwd_stats();
	strlcat(output, ".\n", sizeof(output));
	return output;
}

time_t
Time(time_t *t)
{
	time_t now = time_now;

	if(now == 0)
		now = time(NULL);

	if(t != NULL)
		*t = now;
	return now;
}
