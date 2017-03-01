#include <stdio.h>
#include <string.h>
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
#include "gate.h"

struct active_processes *procs = NULL;

time_t
	time_now = 0,
	Gated_Age_Warn,
	Gated_Age_Kill,
	Gated_Flush;

int
	Gated_Port;

char
	*Bbs_Call,
	*Gated_Bind_Addr = NULL,
	*Gated_File;

char output[4096];
int shutdown_daemon = FALSE;

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "",					tCOMMENT,	NULL },
	{ "GATED_PORT",			tINT,		(int*)&Gated_Port },
	{ "GATED_BIND_ADDR",            tSTRING,        (int*)&Gated_Bind_Addr },
	{ "GATED_FILE",			tFILE,		(int*)&Gated_File },
	{ "",					tCOMMENT,	NULL },
	{ "GATED_AGE_WARN",		tTIME,		(int*)&Gated_Age_Warn },
	{ "GATED_AGE_KILL",		tTIME,		(int*)&Gated_Age_Kill },
	{ NULL, 0, NULL}};

static int
service_port(struct active_processes *ap)
{
    char *c, *s, buf[256];

	if(socket_read_line(ap->fd, buf, 256, 10) == ERROR)
		return ERROR;

	log_f("gated", "R:", buf);
	s = buf;
	NextChar(s);
	if(*s) {
		if((c = parse(ap, s)) == NULL)
			return ERROR;
	} else
		c = Error("invalid command");

	log_f("gated", "S:", c);
	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

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
check_users(struct active_processes *ap, int create)
{
	char buf[80];
	struct gate_entry *g = GateList;

	while(g) {
		if(userd_open() == ERROR)
			return;
		strcpy(buf, userd_fetch(g->call));
		if(!strncmp(buf, "NO,", 3)) {
			snprintf(buf, sizeof(buf), "%s is not a user\n",
				g->call);
			socket_raw_write(ap->fd, buf);

			if(create) {
				printf("%s, is not a user\n", g->call);
				snprintf(buf, sizeof(buf), "CREATE %s",
					g->call);
				userd_fetch(buf);
				sprintf(buf, "HELP 2");
				userd_fetch(buf);
				snprintf(buf, sizeof(buf), "ADDRESS %s",
					g->addr);
				userd_fetch(buf);
				sprintf(buf, "EMAIL ON");
				userd_fetch(buf);
				sprintf(buf, "LOGIN %s", "SMTP");
				userd_fetch(buf);
			}
		}
		userd_close();
		NEXT(g);
	}
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
	time_t flush_time = Time(NULL) + Gated_Flush;

	parse_options(argc, argv, ConfigList, "GATED - Gateway Daemon");

	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

	if(bbsd_open(Bbs_Host, Bbsd_Port, "gated", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
	bbsd_msg("Startup");
	bbsd_sock = bbsd_socket();
	time_now = bbsd_get_time();

	if(read_file() == ERROR)
		error_print_exit(1);

	bbsd_msg(" ");
	listen_port = Gated_Port;
	listen_addr = Gated_Bind_Addr;
	if((listen_sock = socket_listen(listen_addr, &listen_port)) == ERROR)
		exit(1);
	bbsd_port(Gated_Port);

	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;

		if(shutdown_daemon) {
			write_if_needed();
			exit(0);
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

		t.tv_sec = 60;
		t.tv_usec = 0;

		if((cnt = select(fdlimit, (void*)&ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			time_t now = Time(NULL);
			log_clear("gated");
			if(now > flush_time) {
				flush_time = now + Gated_Flush;
				write_if_needed();
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
				if(socket_write(ap->fd, daemon_version("gated", Bbs_Call)) == ERROR)
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
