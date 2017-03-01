#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tcpd.h"

int
	shutdown_daemon = FALSE,
    Msgd_Port, Userd_Port, Wpd_Port, Bidd_Port, Gated_Port,
	Tcpd_Port;

char
    *version,
    *Bbs_Call,
	*Bin_Dir,
	output[4096];

struct active_processes *procs = NULL;

struct ConfigurationList ConfigList[] = {
	{ "", 					tCOMMENT,	NULL},
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host},
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "", 					tCOMMENT,	NULL},
	{ "MSGD_PORT",			tINT,		(int*)&Msgd_Port },
	{ "BIDD_PORT",			tINT,		(int*)&Bidd_Port },
	{ "USERD_PORT",			tINT,		(int*)&Userd_Port },
	{ "WPD_PORT",			tINT,		(int*)&Wpd_Port },
	{ "GATED_PORT",			tINT,		(int*)&Gated_Port },
	{ "", 					tCOMMENT,	NULL},
	{ "TCPD_PORT",			tINT,		(int*)&Tcpd_Port },
	{ "BIN_DIR",			tDIRECTORY,	(int*)&Bin_Dir},
	{ NULL, 				tEND,		NULL}};

static void
display_config(void)
{
	printf("  Bbs_Host = %s\n", Bbs_Host);
	printf(" Bbsd_Port = %d\n", Bbsd_Port);
	printf(" Tcpd_Port = %d\n", Tcpd_Port);
	fflush(stdout);
	exit(0);
}

int
close_downstream(struct active_processes *ap)
{
	if(ap->listen != ERROR) {
		socket_close(ap->listen);
		ap->listen = ERROR;
	}

	if(ap->sock != ERROR) {
		socket_close(ap->sock);
		ap->sock = ERROR;
	}
	strcpy(ap->cmd, "TCPD");
	if(socket_write(ap->fd, version) == ERROR)
		return ERROR;
	return OK;
}

int
service_sock(struct active_processes *ap)
{
    char *c, buf[256];
	int cnt;

#if 0
	do {
		if(socket_read_line(ap->sock, buf, 256, 10) == ERROR)
			return close_downstream(ap);

		if(socket_write(ap->fd, buf) == ERROR)
			return ERROR;
	} while(socket_read_pending(ap->sock) == TRUE);
#endif
	if((cnt = read(ap->sock, buf, 256)) < 0)
		return close_downstream(ap);
	if(write(ap->fd, buf, cnt) < 0)
		return ERROR;
	return OK;
}

int
pass_thru(struct active_processes *ap)
{
	char buf[256];

	do {
		if(socket_read_line(ap->fd, buf, 256, 1) == sockERROR)
			return ERROR;

		if(socket_write(ap->sock, buf) == sockERROR)
			return close_downstream(ap);
	} while(socket_read_pending(ap->sock) == TRUE);
}

int
service_port(struct active_processes *ap)
{
    char *c, buf[256];

	if(ap->sock != ERROR)
		return pass_thru(ap);

	if(socket_read_line(ap->fd, buf, 256, 1) == sockERROR)
		return ERROR;

	logf("tcpd", "R:", buf);
	if((c = parse(ap, buf)) == NULL)
		return ERROR;

	logf("tcpd", "S:", c);
	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

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
	ap->sock = ERROR;
	ap->listen = ERROR;
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

	if(ap->listen != ERROR)
		socket_close(ap->listen);
	if(ap->sock != ERROR)
		socket_close(ap->sock);
    socket_close(ap->fd);
    free(ap);
    return tap;
}

char *
show_status(struct active_processes *ap)
{
	int i = 0;
    struct active_processes *tap = procs;

	output[0] = 0;
	while(tap) {
		sprintf(output, "%s%c%d\t%d\t%d\t%s\n", 
			output, ((long)tap == (long)ap) ? '*':' ',
			tap->fd, tap->sock, tap->listen, tap->cmd);
		NEXT(tap);
	}
	return output;
}

main(int argc, char *argv[])
{
	int listen_sock;
	int bbsd_sock;
	struct timeval t;
	struct active_processes *ap;

	parse_options(argc, argv, ConfigList, "TCPD - TCP Daemon");

	if(!(dbug_level & dbgIGNOREHOST))
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon();
	if(dbug_level & dbgVERBOSE) {
		Logging = logON;
		logf("tcpd", "   ", "UP");
	}

    if(bbsd_open(Bbs_Host, Bbsd_Port, "tcpd", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
	bbsd_sock = bbsd_socket();

	version = daemon_version("tcpd", Bbs_Call);

	if(VERBOSE)
		display_config();

	if((listen_sock = socket_listen(&Tcpd_Port)) == ERROR)
		exit(1);

    bbsd_port(Tcpd_Port);
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;

		error_print();

		if(shutdown_daemon)
			exit(0);

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

				if(ap->listen != ERROR) {
					FD_SET(ap->listen, &ready);
					if(ap->listen > fdlimit)
						fdlimit = ap->listen;
				} else
					if(ap->sock != ERROR) {
						FD_SET(ap->sock, &ready);
						if(ap->sock > fdlimit)
							fdlimit = ap->sock;
					}
			}
			NEXT(ap);
		}
		fdlimit += 1;

		t.tv_sec = 60;
		t.tv_usec = 0;

		if((cnt = select(fdlimit, (void*)&ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			wait3(NULL, WNOHANG, NULL);
			log_clear("tcpd");
			continue;
		}

		if(FD_ISSET(bbsd_sock, &ready))
			shutdown_daemon = TRUE;

        if(FD_ISSET(listen_sock, &ready)) {
            ap = add_proc();
            if((ap->fd = socket_accept(listen_sock)) < 0)
                ap = remove_proc(ap);
			else {
				strcpy(ap->cmd, "TCPD");
				if(socket_write(ap->fd, version) == ERROR)
					ap = remove_proc(ap);
			}
        }

        ap = procs;
        while(ap) {
			if(ap->listen != ERROR) {
				if((ap->sock = socket_accept(ap->listen)) < 0) {
					socket_write(ap->fd, "Connection failed");
					close_downstream(ap);
				}
				else {
					socket_close(ap->listen);	
					ap->listen = ERROR;
				}
			}

			if(ap->sock != ERROR) {
				if(FD_ISSET(ap->sock, &ready)) {
					if(service_sock(ap) == ERROR) {
						ap = remove_proc(ap);
						continue;
					}
				}
			}

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
}
