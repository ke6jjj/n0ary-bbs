#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

struct UserDirectory *UsrDir = NULL;

time_t
	time_now = 0,
	Userd_Age_Suspect,
	Userd_Age_Home,
	Userd_Age_NotHome,
	Userd_Age_NonHam,
	Userd_Age_Interval = (1 * tHour);

int
	Userd_Port;

char
	*Bbs_Call,
	*Userd_Acc_Path,
	*Userd_Bind_Addr = NULL;

struct active_processes *procs = NULL;
int shutdown_daemon = FALSE;
char output[4096];

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "",					tCOMMENT,	NULL },
	{ "USERD_PORT",			tINT,		(int*)&Userd_Port },
	{ "USERD_BIND_ADDR",		tSTRING,	(int*)&Userd_Bind_Addr },
	{ "USERD_ACC_PATH",		tDIRECTORY,	(int*)&Userd_Acc_Path },
	{ "USERD_AGE_INTERVAL",	tTIME,		(int*)&Userd_Age_Interval },
	{ "USERD_AGE_SUSPECT",	tTIME,		(int*)&Userd_Age_Suspect },
	{ "USERD_AGE_HOME",		tTIME,		(int*)&Userd_Age_Home },
	{ "USERD_AGE_NONHAM",	tTIME,		(int*)&Userd_Age_NonHam },
	{ "USERD_AGE_NOTHOME",	tTIME,		(int*)&Userd_Age_NotHome },
	{ NULL, 0, NULL}};

static int
service_port(struct active_processes *ap)
{
	const char *c;
	char *s, buf[256];

	if(socket_read_line(ap->fd, buf, 256, 10) == ERROR)
		return ERROR;

	log_f("userd", "R:", buf);

	s = buf;
	NextChar(s);
	if(*s) {
		if((c = parse(ap, s)) == NULL) {
			return ERROR;
		}
	} else
		c = "OK\n";

	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

	log_f("userd", "S:", c);
	return OK;
}

static struct active_processes *
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
    return ap;
}

static struct active_processes *
remove_proc(struct active_processes *ap)
{
    struct active_processes *tap = procs;

	usrfile_close_all(ap);
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

int
main(int argc, char *argv[])
{
	int listen_port;
	int listen_sock;
	int bbsd_sock;
	struct timeval t;
	struct active_processes *ap;
	time_t age_time = Time(NULL) + Userd_Age_Interval;
	char *bind_addr;

	parse_options(argc, argv, ConfigList, "USERD - User Daemon");

	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

	if(bbsd_open(Bbs_Host, Bbsd_Port, "userd", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
	bbsd_msg("Startup");
	bbsd_sock = bbsd_socket();
	time_now = bbsd_get_time();

	usrdir_build();

	if(dbug_level & dbgVERBOSE)
		printf("UP\n");

	listen_port = Userd_Port;
	bind_addr = Userd_Bind_Addr;
	if((listen_sock = socket_listen(bind_addr, &listen_port)) == ERROR)
		return 1;

    bbsd_port(Userd_Port);
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;

		if(shutdown_daemon == TRUE) {
			return 0;
		}

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

		t.tv_sec = 120;
		t.tv_usec = 0;

		if((cnt = select(fdlimit, (void*)&ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			time_t now = Time(NULL);
			log_clear("userd");
			if(now > age_time) {
				log_f("userd", "X:", "Start aging");
				if(!(dbug_level & dbgNODAEMONS))
					bbsd_msg("Aging users");
				age_users(NULL);
				age_time = now + Userd_Age_Interval;
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
				if(socket_write(ap->fd, daemon_version("userd", Bbs_Call)) == ERROR)
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

	}

	return 0;
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
